#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

/*
	Taken from my game framework
*/

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include <future>
#include <deque>
#include "types.hpp"
#include "log.hpp"

namespace MC {
	enum class TaskPriority { CRITICAL, VERY_HIGH, HIGH, NORMAL, LOW, VERY_LOW, BACKGROUND };

	struct ThreadControlBlock {
		std::thread::id   thread_id;
		std::atomic<bool> is_registered_for_sync{ false };
		std::atomic<bool> has_reached_sync_point{ false };
	};

	class ThreadPool {
	public:
		ThreadPool(u32 num_threads = std::thread::hardware_concurrency());
		~ThreadPool();
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		template<class _Fty, class... Args> std::future<typename std::invoke_result<_Fty, Args...>::type> Enqueue(TaskPriority priority, bool synchronize, _Fty&& f, Args&&... args);
		void                                                                                        SyncThisThread(bool register_for_sync);
		bool                                                                                        SyncRegisteredTasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(500));
		void                                                                                        ExecuteAndWait(const std::vector<std::function<void()>>& tasks);
		void                                                                                        WaitForAllTasks();

	private:
		void                  Initialize(u32 num_threads);
		void                  Shutdown();
		void                  WorkerThread(std::shared_ptr<ThreadControlBlock> tcb, u32 index);
		bool                  CanStealTask() const;
		std::function<void()> StealTask();
		bool                  AllQueuesEmpty() const;
		u32                   SelectQueue();

	private:
		std::vector<std::thread>                                                m_workers;
		std::vector<std::shared_ptr<ThreadControlBlock>>                        m_threads_control;
		std::vector<std::deque<std::pair<TaskPriority, std::function<void()>>>> m_tasks_queues;
		std::mutex                                                              m_queue_mutex;
		std::condition_variable                                                 m_condition;
		std::mutex                                                              m_sync_mutex;
		std::condition_variable                                                 m_sync_condition;
		std::atomic<u64>                                                        m_active_tasks;
		std::atomic<u64>                                                        m_sync_tasks;
		std::atomic<bool>                                                       m_stop;
	};

	template<class _Fty, class... Args> std::future<typename std::invoke_result<_Fty, Args...>::type> ThreadPool::Enqueue(TaskPriority priority, bool synchronize, _Fty&& f, Args&&... args) {
		using return_type = typename std::invoke_result<_Fty, Args...>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<_Fty>(f), std::forward<Args>(args)...));

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);

			if (m_stop) {
				LOG_FATAL("[ THREAD POOL ] Enqueue() called after shutdown");
			}

			u32 queue_index = SelectQueue();
			m_tasks_queues[queue_index].emplace_back(priority, [task, synchronize, this]() {
				try {
					(*task)();
				}
				catch (std::runtime_error& e) {
					LOG_ERROR("[ THREAD POOL ] Task threw an exception" << e.what());
				}
				if (synchronize) {
					// Only decrement sync_tasks if it was a synchronized task
					m_sync_tasks.fetch_sub(1, std::memory_order_release);
					m_sync_condition.notify_all();
				}
				m_active_tasks.fetch_sub(1, std::memory_order_release);
				});
			m_active_tasks.fetch_add(1, std::memory_order_acquire);
			if (synchronize) {
				// Only increment sync_tasks if it's a synchronized task
				m_sync_tasks.fetch_add(1, std::memory_order_acquire);
			}
		}

		m_condition.notify_one();
		return res;
	}
} // namespace Spark

#endif // SPARK_THREAD_POOL_HPP