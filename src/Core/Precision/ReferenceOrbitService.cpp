#include "Core/Precision/ReferenceOrbitService.h"

#include "Core/Precision/HighPrecisionBackend.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mw {

ReferenceOrbitService::ReferenceOrbitService(ReferenceOrbitServiceLimits limits)
    : limits_(limits) {}

bool ReferenceOrbitService::IsSupported(const ReferenceOrbitRequest& request,
                                        std::string& error) const noexcept {
    error.clear();
    const bool selectedDirectTier =
        (request.plan.backend == PrecisionExecutionBackend::CpuBoost512Reference &&
         request.plan.selectedBits == 512) ||
        (request.plan.backend == PrecisionExecutionBackend::CpuBoost2048Direct &&
         request.plan.selectedBits == 2048) ||
        (request.plan.backend == PrecisionExecutionBackend::CpuBoost8192Direct &&
         request.plan.selectedBits == 8192) ||
        (request.plan.backend == PrecisionExecutionBackend::CpuBoost16384Direct &&
         request.plan.selectedBits == 16384);
    if (!selectedDirectTier) {
        error = "Reference-orbit service requires a planner-selected supported Boost CPU tier.";
        return false;
    }
    if (request.plan.version != kPrecisionPlanVersion) {
        error = "Reference-orbit request does not use the current precision-plan version.";
        return false;
    }
    const OrbitEncodingDescriptor& encoding = CurrentOrbitEncoding();
    if (request.orbitEncodingIdentifier != encoding.identifier ||
        request.orbitEncodingVersion != encoding.version) {
        error = "Reference-orbit request does not use the current orbit-encoding contract.";
        return false;
    }
    if (request.plan.formulaProfile == PerturbationProfile::Unsupported) {
        error = "Reference-orbit service requires a registered deep-precision formula profile.";
        return false;
    }
    if (ResolvePerturbationProfile(request.equation) != request.plan.formulaProfile) {
        error = "Reference-orbit request formula does not match its immutable precision plan.";
        return false;
    }
    if (request.maximumIterations < 32 || request.maximumIterations > 4096) {
        error = "Reference-orbit iteration count must be between 32 and 4096.";
        return false;
    }
    if (limits_.maximumCacheBytes == 0U || limits_.maximumEntries == 0U) {
        error = "Reference-orbit service cache limits must be non-zero.";
        return false;
    }
    return true;
}

bool ReferenceOrbitService::SameKey(const Entry& entry,
                                    const ReferenceOrbitRequest& request) noexcept {
    return entry.camera == request.camera && entry.equation == request.equation &&
           entry.maximumIterations == request.maximumIterations &&
           entry.precisionPlanVersion == request.plan.version &&
           entry.selectedBits == request.plan.selectedBits &&
           entry.orbitEncodingIdentifier == request.orbitEncodingIdentifier &&
           entry.orbitEncodingVersion == request.orbitEncodingVersion;
}

std::size_t ReferenceOrbitService::OrbitByteSize(const ReferenceOrbit& orbit) noexcept {
    constexpr std::size_t pointSize = sizeof(ReferenceOrbitPoint);
    if (orbit.points.size() > (std::numeric_limits<std::size_t>::max)() / pointSize) {
        return (std::numeric_limits<std::size_t>::max)();
    }
    return orbit.points.size() * pointSize;
}

void ReferenceOrbitService::PopulateProvenance(const ReferenceOrbitRequest& request,
                                               ReferenceOrbitResult& result) {
    result.plan = request.plan;
    result.precisionPlanVersion = std::string(request.plan.version);
    const PerturbationFormulaCapability capability =
        DescribePerturbationProfile(request.plan.formulaProfile);
    result.formulaCapabilityId = capability.identifier;
    result.formulaCapabilityVersion = capability.version;
    result.orbitEncodingIdentifier = request.orbitEncodingIdentifier;
    result.orbitEncodingVersion = request.orbitEncodingVersion;
}

void ReferenceOrbitService::Insert(Entry entry) noexcept {
    std::lock_guard lock(mutex_);
    if (entry.byteSize > limits_.maximumCacheBytes) return;
    while (!entries_.empty() &&
           (entries_.size() >= limits_.maximumEntries ||
            cachedBytes_ > limits_.maximumCacheBytes - entry.byteSize)) {
        const auto oldest = std::min_element(entries_.begin(), entries_.end(),
            [](const Entry& left, const Entry& right) { return left.lastUse < right.lastUse; });
        cachedBytes_ -= oldest->byteSize;
        entries_.erase(oldest);
    }
    entry.lastUse = ++useCounter_;
    cachedBytes_ += entry.byteSize;
    entries_.push_back(std::move(entry));
}

bool ReferenceOrbitService::Build(const ReferenceOrbitRequest& request,
                                  const ReferenceOrbitCancellationCallback& cancellationCallback,
                                  ReferenceOrbitResult& result,
                                  std::string& error) {
    result = {};
    if (!IsSupported(request, error)) return false;
    if (cancellationCallback && cancellationCallback()) {
        error = "Reference-orbit request was cancelled before execution.";
        return false;
    }
    {
        std::lock_guard lock(mutex_);
        const auto found = std::find_if(entries_.begin(), entries_.end(),
            [&](const Entry& entry) { return SameKey(entry, request); });
        if (found != entries_.end()) {
            found->lastUse = ++useCounter_;
            result.orbit = found->orbit;
            result.generation = request.generation;
            result.cacheHit = true;
            result.byteSize = found->byteSize;
            PopulateProvenance(request, result);
            return true;
        }
    }
    try {
        ReferenceOrbit orbit = BuildIndependentHighPrecisionReferenceOrbit(
            request.camera, request.equation, request.maximumIterations, cancellationCallback,
            request.plan.selectedBits);
        const std::size_t byteSize = OrbitByteSize(orbit);
        if (byteSize > limits_.maximumCacheBytes) {
            error = "Reference orbit exceeds the configured cache budget.";
            return false;
        }
        if (cancellationCallback && cancellationCallback()) {
            error = "Reference-orbit request was cancelled before publication.";
            return false;
        }
        result.orbit = orbit;
        result.generation = request.generation;
        result.byteSize = byteSize;
        PopulateProvenance(request, result);
        Insert({request.camera, request.equation, request.maximumIterations,
                std::string(request.plan.version), request.plan.selectedBits,
                request.orbitEncodingIdentifier, request.orbitEncodingVersion,
                std::move(orbit), byteSize, 0U});
        return true;
    } catch (const std::runtime_error& exception) {
        error = exception.what();
        return false;
    } catch (const std::invalid_argument& exception) {
        error = exception.what();
        return false;
    }
}

void ReferenceOrbitService::Clear() noexcept {
    std::lock_guard lock(mutex_);
    entries_.clear();
    cachedBytes_ = 0U;
}

std::size_t ReferenceOrbitService::CachedBytes() const noexcept {
    std::lock_guard lock(mutex_);
    return cachedBytes_;
}

std::size_t ReferenceOrbitService::CachedEntries() const noexcept {
    std::lock_guard lock(mutex_);
    return entries_.size();
}

ReferenceOrbitWorker::ReferenceOrbitWorker(ReferenceOrbitWorkerLimits limits)
    : limits_(limits), service_(limits.serviceLimits), thread_([this] { Run(); }) {}

ReferenceOrbitWorker::~ReferenceOrbitWorker() {
    Shutdown();
}

bool ReferenceOrbitWorker::SameNumericalKey(const ReferenceOrbitRequest& left,
                                            const ReferenceOrbitRequest& right) noexcept {
    return left.camera == right.camera && left.equation == right.equation &&
           left.maximumIterations == right.maximumIterations &&
           left.plan.version == right.plan.version &&
           left.plan.backend == right.plan.backend &&
           left.plan.selectedBits == right.plan.selectedBits &&
           left.plan.formulaProfile == right.plan.formulaProfile &&
           left.orbitEncodingIdentifier == right.orbitEncodingIdentifier &&
           left.orbitEncodingVersion == right.orbitEncodingVersion;
}

bool ReferenceOrbitWorker::AllCancelledLocked(const Pending& pending) const noexcept {
    return std::all_of(pending.subscribers.begin(), pending.subscribers.end(),
                       [](const Subscriber& subscriber) { return subscriber.cancelled; });
}

std::uint64_t ReferenceOrbitWorker::Enqueue(const ReferenceOrbitRequest& request,
                                            ReferenceOrbitCompletionCallback completion,
                                            std::string& error) {
    error.clear();
    if (!completion) {
        error = "Reference-orbit work requires a completion callback.";
        return 0U;
    }
    std::lock_guard lock(mutex_);
    if (shuttingDown_) {
        error = "Reference-orbit worker is shutting down.";
        return 0U;
    }
    if (limits_.maximumSubscribersPerKey == 0U) {
        error = "Reference-orbit worker subscriber limit is zero.";
        return 0U;
    }
    const std::uint64_t requestId = nextRequestId_++;
    Subscriber subscriber{requestId, request.generation, request.plan, std::move(completion), false};
    enum class CoalesceOutcome { NoMatch, Accepted, Refused };
    const auto coalesce = [&](Pending& pending) {
        if (!SameNumericalKey(pending.request, request)) return CoalesceOutcome::NoMatch;
        if (pending.subscribers.size() >= limits_.maximumSubscribersPerKey) {
            error = "Reference-orbit worker subscriber limit is full.";
            return CoalesceOutcome::Refused;
        }
        pending.subscribers.push_back(std::move(subscriber));
        return CoalesceOutcome::Accepted;
    };
    if (active_) {
        const CoalesceOutcome outcome = coalesce(*active_);
        if (outcome == CoalesceOutcome::Accepted) return requestId;
        if (outcome == CoalesceOutcome::Refused) return 0U;
    }
    for (Pending& pending : pending_) {
        const CoalesceOutcome outcome = coalesce(pending);
        if (outcome == CoalesceOutcome::Accepted) return requestId;
        if (outcome == CoalesceOutcome::Refused) return 0U;
    }
    if (limits_.maximumQueuedRequests == 0U || pending_.size() >= limits_.maximumQueuedRequests) {
        error = "Reference-orbit worker queue is full.";
        return 0U;
    }
    Pending pending;
    pending.request = request;
    pending.subscribers.push_back(std::move(subscriber));
    pending_.push_back(std::move(pending));
    condition_.notify_one();
    return requestId;
}

void ReferenceOrbitWorker::Cancel(std::uint64_t requestId) noexcept {
    if (requestId == 0U) return;
    std::lock_guard lock(mutex_);
    const auto cancelIn = [requestId](Pending& pending) {
        for (Subscriber& subscriber : pending.subscribers) {
            if (subscriber.requestId == requestId) subscriber.cancelled = true;
        }
    };
    if (active_) cancelIn(*active_);
    for (Pending& pending : pending_) cancelIn(pending);
    condition_.notify_one();
}

void ReferenceOrbitWorker::Shutdown() noexcept {
    {
        std::unique_lock lock(mutex_);
        if (shuttingDown_) {
            if (std::this_thread::get_id() == thread_.get_id()) return;
            condition_.wait(lock, [this] { return stopped_; });
            return;
        }
        shuttingDown_ = true;
        for (Pending& pending : pending_) {
            for (Subscriber& subscriber : pending.subscribers) subscriber.cancelled = true;
        }
        if (active_) {
            for (Subscriber& subscriber : active_->subscribers) subscriber.cancelled = true;
        }
    }
    condition_.notify_all();
    thread_.request_stop();
    if (thread_.joinable() && std::this_thread::get_id() != thread_.get_id()) {
        thread_.join();
    }
    {
        std::lock_guard lock(mutex_);
        pending_.clear();
        active_.reset();
        stopped_ = true;
    }
    condition_.notify_all();
}

std::size_t ReferenceOrbitWorker::PendingKeys() const noexcept {
    std::lock_guard lock(mutex_);
    return pending_.size() + (active_ ? 1U : 0U);
}

void ReferenceOrbitWorker::Run() {
    for (;;) {
        ReferenceOrbitRequest activeRequest;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return shuttingDown_ || !pending_.empty(); });
            if (shuttingDown_) {
                stopped_ = true;
                lock.unlock();
                condition_.notify_all();
                return;
            }
            active_ = std::move(pending_.front());
            pending_.pop_front();
            activeRequest = active_->request;
        }

        ReferenceOrbitResult built;
        std::string error;
        const bool succeeded = service_.Build(
            activeRequest,
            [this] {
                std::lock_guard lock(mutex_);
                return shuttingDown_ || !active_ || AllCancelledLocked(*active_);
            },
            built, error);

        Pending completed;
        bool shuttingDown = false;
        {
            std::lock_guard lock(mutex_);
            if (active_) completed = std::move(*active_);
            active_.reset();
            shuttingDown = shuttingDown_;
        }
        if (shuttingDown) return;
        for (const Subscriber& subscriber : completed.subscribers) {
            if (subscriber.cancelled || !subscriber.completion) continue;
            ReferenceOrbitWorkCompletion completion;
            completion.requestId = subscriber.requestId;
            completion.generation = subscriber.generation;
            completion.cancelled = !succeeded && error.find("cancelled") != std::string::npos;
            completion.result = built;
            completion.result.plan = subscriber.plan;
            completion.result.generation = subscriber.generation;
            completion.error = error;
            subscriber.completion(std::move(completion));
        }
    }
}

} // namespace mw
