#include "thread_pool.hpp"
#include <random>

namespace MC {
	ThreadPool::ThreadPool(u32 num_threads)
		: m_stop(false)
		, m_active_tasks(0)
		, m_sync_tasks(0) {
		Initialize(num_threads);
	}

	ThreadPool::~ThreadPool() { Shutdown(); }

	void ThreadPool::SyncThisThread(bool register_for_sync) {
		std::lock_guard<std::mutex> lock(m_sync_mutex);
		auto it = std::find_if(m_threads_control.begin(), m_threads_control.end(), [](const std::shared_ptr<ThreadControlBlock>& tcb) { return tcb->thread_id == std::this_thread::get_id(); });

		if (it != m_threads_control.end()) {
			(*it)->is_registered_for_sync.store(register_for_sync, std::memory_order_release);
			(*it)->has_reached_sync_point.store(!register_for_sync, std::memory_order_release);
		}
		else {
			LOG_FATAL("[ THREAD POOL ] Current thread is not part of the thread pool");
		}
		m_sync_condition.notify_all();
	}

	bool ThreadPool::SyncRegisteredTasks(std::chrono::milliseconds timeout) {
		std::unique_lock<std::mutex> lock(m_sync_mutex);
		auto                         start = std::chrono::steady_clock::now();
		bool                         synced = m_sync_condition.wait_for(lock, timeout, [this, &start] {
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

			// Only wait for sync_tasks to become 0, ignore non-synchronized tasks
			bool all_synced = (m_sync_tasks.load(std::memory_order_acquire) == 0);

			return all_synced;
			});

		if (synced) {
			for (auto& tcb : m_threads_control) {
				tcb->is_registered_for_sync.store(false, std::memory_order_release);
				tcb->has_reached_sync_point.store(false, std::memory_order_release);
			}
		}
		else {
			LOG_ERROR("[ THREAD POOL ] Sync timed out after " << timeout.count() << "ms");
		}

		return synced;
	}

	void ThreadPool::ExecuteAndWait(const std::vector<std::function<void()>>& tasks) {
		std::atomic<size_t > completed_tasks(0);
		size_t               total_tasks = tasks.size();

		for (const auto& task : tasks) {
			Enqueue(TaskPriority::NORMAL, false, [&completed_tasks, task]() {
				task();
				completed_tasks.fetch_add(1, std::memory_order_release);
				});
		}

		// Wait for all tasks to complete
		while (completed_tasks.load(std::memory_order_acquire) < total_tasks) {
			std::this_thread::yield();
		}
	}

	void ThreadPool::WaitForAllTasks() {
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		m_sync_condition.wait(lock, [this] { return m_active_tasks.load(std::memory_order_acquire) == 0 && AllQueuesEmpty(); });
	}

	void ThreadPool::Initialize(u32 num_threads) {
		m_tasks_queues.resize(num_threads);
		m_threads_control.resize(num_threads);

		for (u32 i = 0; i < num_threads; ++i) {
			m_threads_control[i] = std::make_shared<ThreadControlBlock>();
			m_workers.emplace_back(&ThreadPool::WorkerThread, this, m_threads_control[i], i);
		}
	}

	void ThreadPool::Shutdown() {
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);
			m_stop = true;
		}
		m_condition.notify_all();
		for (std::thread& worker : m_workers) {
			worker.join();
		}
	}

	void ThreadPool::WorkerThread(std::shared_ptr<ThreadControlBlock> tcb, u32 index) {
		tcb->thread_id = std::this_thread::get_id();

		while (true) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(m_queue_mutex);
				m_condition.wait(lock, [this, index] { return m_stop || !m_tasks_queues[index].empty() || CanStealTask(); });

				if (m_stop && AllQueuesEmpty()) {
					return;
				}

				if (!m_tasks_queues[index].empty()) {
					task = std::move(m_tasks_queues[index].front().second);
					m_tasks_queues[index].pop_front();
				}
				else {
					task = StealTask();
				}
			}

			if (task) {
				task();
			}

			if (tcb->is_registered_for_sync.load(std::memory_order_acquire)) {
				tcb->has_reached_sync_point.store(true, std::memory_order_release);
				m_sync_condition.notify_all();
			}
		}
	}

	bool ThreadPool::CanStealTask() const {
		return std::any_of(m_tasks_queues.begin(), m_tasks_queues.end(), [](const auto& queue) { return !queue.empty(); });
	}

	std::function<void()> ThreadPool::StealTask() {
		std::function<void()> task;
		for (auto& queue : m_tasks_queues) {
			if (!queue.empty()) {
				task = std::move(queue.front().second);
				queue.pop_front();
				break;
			}
		}
		return task;
	}

	bool ThreadPool::AllQueuesEmpty() const {
		return std::all_of(m_tasks_queues.begin(), m_tasks_queues.end(), [](const auto& queue) { return queue.empty(); });
	}

	u32 ThreadPool::SelectQueue() {
		static thread_local std::mt19937   gen(std::random_device{}());
		std::uniform_int_distribution<u32> dis(0, m_tasks_queues.size() - 1);
		return dis(gen);
	}
} // namespace Spark