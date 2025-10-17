#pragma once

#include "engine/assets/handles.hpp"
#include "engine/core/diagnostics/error.hpp"
#include "engine/core/diagnostics/result.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/core/threading/io_thread_pool.hpp"

namespace engine::assets {

    enum class AssetType : int
    {
        unknown = 0,
        mesh,
        graph,
        point_cloud,
        texture,
        shader,
        material
    };

    [[nodiscard]] constexpr std::string_view to_string(AssetType type) noexcept
    {
        switch (type)
        {
        case AssetType::unknown:
            return "unknown";
        case AssetType::mesh:
            return "mesh";
        case AssetType::graph:
            return "graph";
        case AssetType::point_cloud:
            return "point_cloud";
        case AssetType::texture:
            return "texture";
        case AssetType::shader:
            return "shader";
        case AssetType::material:
            return "material";
        }

        return "invalid";
    }

    enum class AssetLoadPriority : int
    {
        High = 0,
        Normal,
        Low
    };

    [[nodiscard]] constexpr std::string_view to_string(AssetLoadPriority priority) noexcept
    {
        switch (priority)
        {
        case AssetLoadPriority::High:
            return "high";
        case AssetLoadPriority::Normal:
            return "normal";
        case AssetLoadPriority::Low:
            return "low";
        }

        return "unknown";
    }

    struct AssetImportParams
    {
        std::string format_hint{};
        std::unordered_map<std::string, std::string> metadata{};
        std::vector<std::string> dependency_overrides{};

        void set_format_hint(std::string hint)
        {
            format_hint = std::move(hint);
        }

        void set_metadata(std::string key, std::string value)
        {
            metadata.insert_or_assign(std::move(key), std::move(value));
        }

        [[nodiscard]] std::optional<std::string> get_metadata(std::string_view key) const
        {
            if (const auto it = metadata.find(std::string{key}); it != metadata.end())
            {
                return it->second;
            }

            return std::nullopt;
        }

        void add_dependency_override(std::string identifier)
        {
            dependency_overrides.emplace_back(std::move(identifier));
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return format_hint.empty() && metadata.empty() && dependency_overrides.empty();
        }
    };

    struct AssetLoadRequest
    {
        AssetType type{AssetType::unknown};
        std::string identifier{};
        AssetImportParams import_params{};
        AssetLoadPriority priority{AssetLoadPriority::Normal};
        std::optional<std::chrono::steady_clock::duration> deadline{};
        bool allow_blocking_fallback{false};

        [[nodiscard]] bool has_deadline() const noexcept
        {
            return deadline.has_value();
        }

        [[nodiscard]] static AssetLoadRequest from_identifier(
            AssetType type, std::string identifier, AssetImportParams params = {},
            AssetLoadPriority priority = AssetLoadPriority::Normal,
            std::optional<std::chrono::steady_clock::duration> deadline = std::nullopt,
            bool allow_blocking_fallback = false)
        {
            AssetLoadRequest request;
            request.type = type;
            request.identifier = std::move(identifier);
            request.import_params = std::move(params);
            request.priority = priority;
            request.deadline = deadline;
            request.allow_blocking_fallback = allow_blocking_fallback;
            return request;
        }

        [[nodiscard]] static AssetLoadRequest from_path(
            AssetType type, const std::filesystem::path& path, AssetImportParams params = {},
            AssetLoadPriority priority = AssetLoadPriority::Normal,
            std::optional<std::chrono::steady_clock::duration> deadline = std::nullopt,
            bool allow_blocking_fallback = false)
        {
            if (params.format_hint.empty())
            {
                params.format_hint = path.extension().string();
            }

            return from_identifier(type, path.generic_string(), std::move(params), priority,
                                   deadline, allow_blocking_fallback);
        }
    };

    enum class AssetLoadState : int
    {
        Pending = 0,
        Loading,
        Ready,
        Failed,
        Cancelled
    };

    [[nodiscard]] constexpr std::string_view to_string(AssetLoadState state) noexcept
    {
        switch (state)
        {
        case AssetLoadState::Pending:
            return "pending";
        case AssetLoadState::Loading:
            return "loading";
        case AssetLoadState::Ready:
            return "ready";
        case AssetLoadState::Failed:
            return "failed";
        case AssetLoadState::Cancelled:
            return "cancelled";
        }

        return "unknown";
    }

    struct ProgressInfo
    {
        std::size_t bytes_transferred{0};
        std::size_t total_bytes{0};
        std::uint32_t dependencies_total{0};
        std::uint32_t dependencies_completed{0};

        [[nodiscard]] bool has_known_total() const noexcept
        {
            return total_bytes != 0;
        }

        [[nodiscard]] double completion_ratio() const noexcept
        {
            if (!has_known_total())
            {
                return 0.0;
            }

            return static_cast<double>(bytes_transferred) / static_cast<double>(total_bytes);
        }
    };

    enum class AssetLoadErrorCategory : int
    {
        IoFailure = 1,
        DecodeError,
        ValidationError,
        Cancelled,
        Timeout
    };

    [[nodiscard]] constexpr std::string_view to_string(AssetLoadErrorCategory category) noexcept
    {
        switch (category)
        {
        case AssetLoadErrorCategory::IoFailure:
            return "io_failure";
        case AssetLoadErrorCategory::DecodeError:
            return "decode_error";
        case AssetLoadErrorCategory::ValidationError:
            return "validation_error";
        case AssetLoadErrorCategory::Cancelled:
            return "cancelled";
        case AssetLoadErrorCategory::Timeout:
            return "timeout";
        }

        return "unknown";
    }

    class AssetLoadError final : public engine::EnumeratedErrorCode<AssetLoadErrorCategory>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] AssetLoadError with_message(std::string message) const
        {
            AssetLoadError copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline AssetLoadError make_asset_load_error(AssetLoadErrorCategory category,
                                                              std::string message = {})
    {
        AssetLoadError error{"engine.assets", category, to_string(category)};
        if (!message.empty())
        {
            error = error.with_message(std::move(message));
        }
        return error;
    }

    inline std::ostream& operator<<(std::ostream& stream, AssetLoadState state)
    {
        return stream << to_string(state);
    }

    inline std::ostream& operator<<(std::ostream& stream, AssetLoadErrorCategory category)
    {
        return stream << to_string(category);
    }

    inline std::ostream& operator<<(std::ostream& stream, AssetType type)
    {
        return stream << to_string(type);
    }

    inline std::ostream& operator<<(std::ostream& stream, AssetLoadPriority priority)
    {
        return stream << to_string(priority);
    }

    template <typename Handle>
    using AssetLoadResult = engine::Result<Handle, AssetLoadError>;

    template <typename Handle>
    class AssetLoadFuture;

    namespace detail {
        template <typename Handle>
        struct AssetLoadSharedState
        {
            mutable std::mutex mutex;
            std::condition_variable condition;
            AssetLoadState state{AssetLoadState::Pending};
            ProgressInfo progress{};
            bool cancellation_requested{false};
            std::optional<AssetLoadResult<Handle>> outcome{};
            std::function<void()> cancellation_callback{};
        };

        template <typename Handle>
        class AssetLoadPromise;

        template <typename Handle>
        std::pair<AssetLoadPromise<Handle>, AssetLoadFuture<Handle>> make_asset_load_channel();
    } // namespace detail

    template <typename Handle>
    class AssetLoadFuture
    {
    public:
        using handle_type = Handle;
        using result_type = AssetLoadResult<Handle>;

        AssetLoadFuture() = default;

        [[nodiscard]] bool valid() const noexcept
        {
            return static_cast<bool>(state_);
        }

        [[nodiscard]] AssetLoadState state() const
        {
            if (!state_)
            {
                return AssetLoadState::Cancelled;
            }

            std::scoped_lock lock{state_->mutex};
            return state_->state;
        }

        [[nodiscard]] bool is_ready() const
        {
            if (!state_)
            {
                return true;
            }

            std::scoped_lock lock{state_->mutex};
            return is_terminal_state_unlocked();
        }

        void wait() const
        {
            if (!state_)
            {
                return;
            }

            std::unique_lock lock{state_->mutex};
            state_->condition.wait(lock, [this]() {
                return is_terminal_state_unlocked();
            });
        }

        [[nodiscard]] result_type get() const
        {
            if (!state_)
            {
                return result_type{make_asset_load_error(AssetLoadErrorCategory::ValidationError,
                                                         "invalid future")};
            }

            wait();

            std::scoped_lock lock{state_->mutex};
            if (!state_->outcome.has_value())
            {
                return result_type{make_asset_load_error(AssetLoadErrorCategory::ValidationError,
                                                         "missing outcome")};
            }

            return state_->outcome.value();
        }

        [[nodiscard]] ProgressInfo progress() const
        {
            if (!state_)
            {
                return {};
            }

            std::scoped_lock lock{state_->mutex};
            return state_->progress;
        }

        void cancel()
        {
            if (!state_)
            {
                return;
            }

            std::function<void()> callback;
            {
                std::lock_guard lock{state_->mutex};
                if (state_->cancellation_requested)
                {
                    return;
                }

                state_->cancellation_requested = true;
                callback = state_->cancellation_callback;
            }

            if (callback)
            {
                callback();
            }
        }

        [[nodiscard]] bool cancellation_requested() const
        {
            if (!state_)
            {
                return false;
            }

            std::scoped_lock lock{state_->mutex};
            return state_->cancellation_requested;
        }

    private:
        friend class detail::AssetLoadPromise<Handle>;
        friend std::pair<detail::AssetLoadPromise<Handle>, AssetLoadFuture<Handle>>
        detail::make_asset_load_channel<Handle>();

        explicit AssetLoadFuture(std::shared_ptr<detail::AssetLoadSharedState<Handle>> state)
            : state_{std::move(state)}
        {
        }

        [[nodiscard]] bool is_terminal_state_unlocked() const noexcept
        {
            return state_->state == AssetLoadState::Ready || state_->state == AssetLoadState::Failed ||
                   state_->state == AssetLoadState::Cancelled;
        }

        std::shared_ptr<detail::AssetLoadSharedState<Handle>> state_{};
    };

    namespace detail {
        template <typename Handle>
        class AssetLoadPromise
        {
        public:
            using future_type = AssetLoadFuture<Handle>;
            using handle_type = Handle;
            using result_type = AssetLoadResult<Handle>;

            AssetLoadPromise() = default;

            explicit AssetLoadPromise(std::shared_ptr<AssetLoadSharedState<Handle>> state)
                : state_{std::move(state)}
            {
            }

            [[nodiscard]] bool valid() const noexcept
            {
                return static_cast<bool>(state_);
            }

            [[nodiscard]] future_type get_future() const
            {
                return future_type{state_};
            }

            void set_loading()
            {
                if (!state_)
                {
                    return;
                }

                std::lock_guard lock{state_->mutex};
                if (state_->state == AssetLoadState::Pending)
                {
                    state_->state = AssetLoadState::Loading;
                }
            }

            void set_ready(handle_type handle)
            {
                set_result(result_type{std::move(handle)}, AssetLoadState::Ready);
            }

            void set_failed(AssetLoadError error)
            {
                set_result(result_type{std::move(error)}, AssetLoadState::Failed);
            }

            void set_cancelled(AssetLoadError error =
                                   make_asset_load_error(AssetLoadErrorCategory::Cancelled))
            {
                set_result(result_type{std::move(error)}, AssetLoadState::Cancelled);
            }

            void update_progress(const ProgressInfo& info)
            {
                if (!state_)
                {
                    return;
                }

                std::lock_guard lock{state_->mutex};
                state_->progress = info;
            }

            void set_cancellation_callback(std::function<void()> callback)
            {
                if (!state_)
                {
                    return;
                }

                std::lock_guard lock{state_->mutex};
                state_->cancellation_callback = std::move(callback);
            }

            [[nodiscard]] AssetLoadState state() const
            {
                if (!state_)
                {
                    return AssetLoadState::Cancelled;
                }

                std::scoped_lock lock{state_->mutex};
                return state_->state;
            }

            [[nodiscard]] bool cancellation_requested() const
            {
                if (!state_)
                {
                    return false;
                }

                std::scoped_lock lock{state_->mutex};
                return state_->cancellation_requested;
            }

        private:
            void set_result(result_type result, AssetLoadState terminal_state)
            {
                if (!state_)
                {
                    return;
                }

                std::lock_guard lock{state_->mutex};
                if (state_->state == AssetLoadState::Ready || state_->state == AssetLoadState::Failed ||
                    state_->state == AssetLoadState::Cancelled)
                {
                    return;
                }

                state_->state = terminal_state;
                state_->outcome = std::move(result);
                state_->cancellation_callback = nullptr;
                state_->condition.notify_all();
            }

            std::shared_ptr<AssetLoadSharedState<Handle>> state_{};
        };

        template <typename Handle>
        std::pair<AssetLoadPromise<Handle>, AssetLoadFuture<Handle>> make_asset_load_channel()
        {
            auto state = std::make_shared<AssetLoadSharedState<Handle>>();
            AssetLoadPromise<Handle> promise{state};
            AssetLoadFuture<Handle> future{std::move(state)};
            return {std::move(promise), std::move(future)};
        }
    } // namespace detail

    struct AssetStreamingSnapshot
    {
        std::uint64_t pending{0};
        std::uint64_t loading{0};
        std::uint64_t total_requests{0};
        std::uint64_t total_completed{0};
        std::uint64_t total_failed{0};
        std::uint64_t total_cancelled{0};
        std::uint64_t total_rejected{0};
    };

    class AssetStreamingTelemetry
    {
    public:
        static AssetStreamingTelemetry& instance()
        {
            static AssetStreamingTelemetry telemetry;
            return telemetry;
        }

        void on_enqueued()
        {
            pending_.fetch_add(1, std::memory_order_relaxed);
            total_requests_.fetch_add(1, std::memory_order_relaxed);
        }

        void on_transition(AssetLoadState from, AssetLoadState to)
        {
            if (from == to)
            {
                return;
            }

            decrement_state(from);
            increment_state(to);

            switch (to)
            {
            case AssetLoadState::Ready:
                total_completed_.fetch_add(1, std::memory_order_relaxed);
                break;
            case AssetLoadState::Failed:
                total_failed_.fetch_add(1, std::memory_order_relaxed);
                break;
            case AssetLoadState::Cancelled:
                total_cancelled_.fetch_add(1, std::memory_order_relaxed);
                break;
            default:
                break;
            }
        }

        void on_rejected()
        {
            total_rejected_.fetch_add(1, std::memory_order_relaxed);
        }

        [[nodiscard]] AssetStreamingSnapshot snapshot() const noexcept
        {
            AssetStreamingSnapshot snapshot{};
            snapshot.pending = pending_.load(std::memory_order_relaxed);
            snapshot.loading = loading_.load(std::memory_order_relaxed);
            snapshot.total_requests = total_requests_.load(std::memory_order_relaxed);
            snapshot.total_completed = total_completed_.load(std::memory_order_relaxed);
            snapshot.total_failed = total_failed_.load(std::memory_order_relaxed);
            snapshot.total_cancelled = total_cancelled_.load(std::memory_order_relaxed);
            snapshot.total_rejected = total_rejected_.load(std::memory_order_relaxed);
            return snapshot;
        }

    private:
        AssetStreamingTelemetry() = default;

        void increment_state(AssetLoadState state)
        {
            switch (state)
            {
            case AssetLoadState::Pending:
                pending_.fetch_add(1, std::memory_order_relaxed);
                break;
            case AssetLoadState::Loading:
                loading_.fetch_add(1, std::memory_order_relaxed);
                break;
            default:
                break;
            }
        }

        void decrement_state(AssetLoadState state)
        {
            switch (state)
            {
            case AssetLoadState::Pending:
                pending_.fetch_sub(1, std::memory_order_relaxed);
                break;
            case AssetLoadState::Loading:
                loading_.fetch_sub(1, std::memory_order_relaxed);
                break;
            default:
                break;
            }
        }

        std::atomic<std::uint64_t> pending_{0};
        std::atomic<std::uint64_t> loading_{0};
        std::atomic<std::uint64_t> total_requests_{0};
        std::atomic<std::uint64_t> total_completed_{0};
        std::atomic<std::uint64_t> total_failed_{0};
        std::atomic<std::uint64_t> total_cancelled_{0};
        std::atomic<std::uint64_t> total_rejected_{0};
    };

    inline bool is_terminal_state(AssetLoadState state) noexcept
    {
        return state == AssetLoadState::Ready || state == AssetLoadState::Failed ||
               state == AssetLoadState::Cancelled;
    }

    inline core::threading::IoTaskPriority to_io_task_priority(AssetLoadPriority priority) noexcept
    {
        switch (priority)
        {
        case AssetLoadPriority::High:
            return core::threading::IoTaskPriority::High;
        case AssetLoadPriority::Low:
            return core::threading::IoTaskPriority::Low;
        case AssetLoadPriority::Normal:
        default:
            return core::threading::IoTaskPriority::Normal;
        }
    }

    template <typename Handle>
    class AssetAsyncQueue
    {
    public:
        using Task = std::function<AssetLoadResult<Handle>(detail::AssetLoadPromise<Handle>&)>;

        AssetAsyncQueue() = default;

        [[nodiscard]] AssetLoadFuture<Handle> schedule(
            std::string identifier,
            AssetLoadPriority priority,
            bool allow_blocking_fallback,
            Task task,
            core::threading::IoThreadPool& pool)
        {
            {
                std::lock_guard lock{mutex_};
                if (auto it = states_.find(identifier); it != states_.end())
                {
                    if (it->second == AssetLoadState::Pending || it->second == AssetLoadState::Loading)
                    {
                        return futures_[identifier];
                    }
                }
            }

            auto channel = detail::make_asset_load_channel<Handle>();
            auto promise_ptr = std::make_shared<detail::AssetLoadPromise<Handle>>(std::move(channel.first));
            AssetLoadFuture<Handle> future = channel.second;
            register_pending(identifier, future);

            auto task_ptr = std::make_shared<Task>(std::move(task));

            auto runner = std::make_shared<std::function<void()>>([this, identifier, promise_ptr, task_ptr]() mutable {
                auto& promise_ref = *promise_ptr;

                if (promise_ref.cancellation_requested())
                {
                    transition(identifier, AssetLoadState::Cancelled);
                    promise_ref.set_cancelled();
                    return;
                }

                promise_ref.set_loading();
                transition(identifier, AssetLoadState::Loading);

                if (promise_ref.cancellation_requested())
                {
                    transition(identifier, AssetLoadState::Cancelled);
                    promise_ref.set_cancelled();
                    return;
                }

                auto result = (*task_ptr)(promise_ref);
                if (!result.has_value())
                {
                    promise_ref.set_failed(result.error());
                    transition(identifier, AssetLoadState::Failed);
                    return;
                }

                if (promise_ref.cancellation_requested())
                {
                    promise_ref.set_cancelled();
                    transition(identifier, AssetLoadState::Cancelled);
                    return;
                }

                promise_ref.set_ready(result.value());
                transition(identifier, AssetLoadState::Ready);
            });

            promise_ptr->set_cancellation_callback([weak_runner = std::weak_ptr<std::function<void()>>(runner),
                                                    identifier,
                                                    this]() {
                if (auto locked = weak_runner.lock())
                {
                    (void)locked;
                    transition(identifier, AssetLoadState::Cancelled);
                }
            });

            const auto io_priority = to_io_task_priority(priority);
            if (!pool.enqueue(io_priority, [runner]() { (*runner)(); }))
            {
                if (allow_blocking_fallback)
                {
                    (*runner)();
                }
                else
                {
                    promise_ptr->set_failed(
                        make_asset_load_error(AssetLoadErrorCategory::Timeout, "IO queue saturated"));
                    transition(identifier, AssetLoadState::Failed);
                    AssetStreamingTelemetry::instance().on_rejected();
                }
            }

            return future;
        }

        [[nodiscard]] AssetLoadState state(std::string_view identifier) const
        {
            std::lock_guard lock{mutex_};
            if (auto it = states_.find(std::string{identifier}); it != states_.end())
            {
                return it->second;
            }
            return AssetLoadState::Ready;
        }

    private:
        void register_pending(const std::string& identifier, const AssetLoadFuture<Handle>& future)
        {
            {
                std::lock_guard lock{mutex_};
                futures_[identifier] = future;
                states_[identifier] = AssetLoadState::Pending;
            }

            AssetStreamingTelemetry::instance().on_enqueued();
        }

        void transition(const std::string& identifier, AssetLoadState next)
        {
            AssetLoadState previous = AssetLoadState::Pending;
            {
                std::lock_guard lock{mutex_};
                auto it = states_.find(identifier);
                if (it != states_.end())
                {
                    previous = it->second;
                    it->second = next;
                }
                else
                {
                    states_[identifier] = next;
                    previous = next;
                }

                if (is_terminal_state(next))
                {
                    futures_.erase(identifier);
                }
            }

            AssetStreamingTelemetry::instance().on_transition(previous, next);
        }

        mutable std::mutex mutex_{};
        std::unordered_map<std::string, AssetLoadFuture<Handle>> futures_{};
        std::unordered_map<std::string, AssetLoadState> states_{};
    };

} // namespace engine::assets

