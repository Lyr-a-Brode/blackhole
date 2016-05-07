#include <atomic>
#include <thread>

#include <cds/container/vyukov_mpmc_cycle_queue.h>

#include "blackhole/sink.hpp"

#include "blackhole/detail/recordbuf.hpp"

namespace blackhole {
inline namespace v1 {
namespace experimental {
namespace sink {

using detail::recordbuf_t;

/// I can imagine: drop, sleep, wait.
class overflow_policy_t {
public:
    enum class action_t {
        retry,
        drop
    };

public:
    virtual ~overflow_policy_t() {}

    /// Handles record queue overflow.
    ///
    /// This method is called when the queue is unable to enqueue more items. It's okay to throw
    /// exceptions from here, they will be propagated directly to the sink caller.
    virtual auto overflow() -> action_t = 0;

    virtual auto wakeup() -> void = 0;
};

class asynchronous_t : public sink_t {
    struct value_type {
        recordbuf_t record;
        std::string message;
    };

    typedef cds::container::VyukovMPSCCycleQueue<value_type> queue_type;

    queue_type queue;
    std::atomic<bool> stopped;
    std::unique_ptr<sink_t> wrapped;

    std::unique_ptr<overflow_policy_t> overflow_policy;

    std::thread thread;

public:
    /// \param factor queue capacity factor from which the queue capacity will be generated as
    ///     a value of `2 << factor`. Must fit in [0; 20] range (1048576 items).
    /// \throw std::invalid_argument if the factor is greater than 20.
    asynchronous_t(std::unique_ptr<sink_t> wrapped, std::size_t factor = 10);

    /// \param factor queue capacity factor from which the queue capacity will be generated as
    ///     a value of `2 << factor`. Must fit in [0; 20] range (1048576 items).
    /// \param queue_type
    /// \param overflow_policy [drop silently, drop with error, block]
    ///
    // asynchronous_t(std::unique_ptr<sink_t> sink,
    //                std::unique_ptr<filter_t> filter,
    //                std::unique_ptr<overflow_policy_t> overflow_policy,
    //                std::unique_ptr<exception_policy_t> exception_policy,
    //                std::size_t factor = 10);

    ~asynchronous_t();

    auto emit(const record_t& record, const string_view& message) -> void;

private:
    auto run() -> void;
};

}  // namespace sink
}  // namespace experimental
}  // namespace v1
}  // namespace blackhole