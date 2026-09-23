#pragma once

#include "Core/Precision/ExactCamera.h"
#include "Core/Precision/PrecisionPlanner.h"
#include "Core/DeepZoom.h"

#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace mw {

// A caller supplies this with every request. It keeps cache reuse separate
// from render ownership: a result may be reused numerically, but is accepted
// by a renderer only when its returned generation matches that renderer.
struct ReferenceOrbitRequest {
    ExactCamera camera;
    EquationSettings equation;
    PrecisionPlan plan;
    int maximumIterations{0};
    std::uint64_t generation{0U};
    std::string orbitEncodingIdentifier{CurrentOrbitEncoding().identifier};
    int orbitEncodingVersion{CurrentOrbitEncoding().version};
};

struct ReferenceOrbitResult {
    ReferenceOrbit orbit;
    PrecisionPlan plan;
    std::uint64_t generation{0U};
    bool cacheHit{false};
    std::size_t byteSize{0U};
    std::string precisionPlanVersion;
    std::string formulaCapabilityId;
    int formulaCapabilityVersion{0};
    std::string orbitEncodingIdentifier;
    int orbitEncodingVersion{0};
};

struct ReferenceOrbitServiceLimits {
    std::size_t maximumCacheBytes{4U * 1024U * 1024U};
    std::size_t maximumEntries{16U};
};

using ReferenceOrbitCancellationCallback = std::function<bool()>;

// Bounded, platform-neutral cache/service foundation for the accepted
// 512-bit CPU reference tier. It never reinterprets a plan: unsupported or
// over-budget requests fail with an error. Worker-pool coalescing is a later
// layer; this synchronous base gives callers deterministic ownership,
// cancellation and cache semantics first.
class ReferenceOrbitService {
public:
    explicit ReferenceOrbitService(ReferenceOrbitServiceLimits limits = {});

    [[nodiscard]] bool Build(const ReferenceOrbitRequest& request,
                             const ReferenceOrbitCancellationCallback& cancellationCallback,
                             ReferenceOrbitResult& result,
                             std::string& error);

    void Clear() noexcept;
    [[nodiscard]] std::size_t CachedBytes() const noexcept;
    [[nodiscard]] std::size_t CachedEntries() const noexcept;

private:
    struct Entry {
        ExactCamera camera;
        EquationSettings equation;
        int maximumIterations{0};
        std::string precisionPlanVersion;
        int selectedBits{0};
        std::string orbitEncodingIdentifier;
        int orbitEncodingVersion{0};
        ReferenceOrbit orbit;
        std::size_t byteSize{0U};
        std::uint64_t lastUse{0U};
    };

    [[nodiscard]] bool IsSupported(const ReferenceOrbitRequest& request,
                                   std::string& error) const noexcept;
    [[nodiscard]] static bool SameKey(const Entry& entry,
                                      const ReferenceOrbitRequest& request) noexcept;
    [[nodiscard]] static std::size_t OrbitByteSize(const ReferenceOrbit& orbit) noexcept;
    static void PopulateProvenance(const ReferenceOrbitRequest& request,
                                   ReferenceOrbitResult& result);
    void Insert(Entry entry) noexcept;

    ReferenceOrbitServiceLimits limits_;
    mutable std::mutex mutex_;
    std::vector<Entry> entries_;
    std::size_t cachedBytes_{0U};
    std::uint64_t useCounter_{0U};
};

// A single bounded worker makes ordering deterministic while keeping costly
// reference generation off render callers. Equal numerical requests coalesce;
// each subscriber retains its own generation, which is stamped only when the
// completed numerical result is delivered.
struct ReferenceOrbitWorkerLimits {
    ReferenceOrbitServiceLimits serviceLimits{};
    std::size_t maximumQueuedRequests{8U};
    std::size_t maximumSubscribersPerKey{64U};
};

struct ReferenceOrbitWorkCompletion {
    std::uint64_t requestId{0U};
    std::uint64_t generation{0U};
    bool cancelled{false};
    ReferenceOrbitResult result;
    std::string error;
};

using ReferenceOrbitCompletionCallback = std::function<void(ReferenceOrbitWorkCompletion)>;

class ReferenceOrbitWorker {
public:
    explicit ReferenceOrbitWorker(ReferenceOrbitWorkerLimits limits = {});
    ~ReferenceOrbitWorker();

    ReferenceOrbitWorker(const ReferenceOrbitWorker&) = delete;
    ReferenceOrbitWorker& operator=(const ReferenceOrbitWorker&) = delete;

    // Returns zero when a bounded key queue or subscriber list cannot accept
    // the request. Equal numerical requests coalesce only within their
    // configured per-key subscriber cap.
    [[nodiscard]] std::uint64_t Enqueue(const ReferenceOrbitRequest& request,
                                        ReferenceOrbitCompletionCallback completion,
                                        std::string& error);
    void Cancel(std::uint64_t requestId) noexcept;
    // Refuses new work, cancels all subscribers and returns only after the
    // owned worker has stopped and released queued/active request state.
    // A completion callback must not call Shutdown on this same worker thread.
    void Shutdown() noexcept;
    [[nodiscard]] std::size_t PendingKeys() const noexcept;

private:
    struct Subscriber {
        std::uint64_t requestId{0U};
        std::uint64_t generation{0U};
        PrecisionPlan plan;
        ReferenceOrbitCompletionCallback completion;
        bool cancelled{false};
    };
    struct Pending {
        ReferenceOrbitRequest request;
        std::vector<Subscriber> subscribers;
    };

    [[nodiscard]] static bool SameNumericalKey(const ReferenceOrbitRequest& left,
                                               const ReferenceOrbitRequest& right) noexcept;
    [[nodiscard]] bool AllCancelledLocked(const Pending& pending) const noexcept;
    void Run();

    ReferenceOrbitWorkerLimits limits_;
    ReferenceOrbitService service_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Pending> pending_;
    std::optional<Pending> active_;
    std::jthread thread_;
    std::uint64_t nextRequestId_{1U};
    bool shuttingDown_{false};
    bool stopped_{false};
};

} // namespace mw
