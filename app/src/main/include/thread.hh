//
// Created by ttand on 20-1-6.
//

#ifndef STBOX_THREAD_HH
#define STBOX_THREAD_HH

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace tt {

	class Thread {
		int status = 1;
		std::queue<std::function<void()>> jobQueue;
		std::mutex queueMutex;
		std::condition_variable condition;
		int in = 0;
		// Loop through all remaining jobs
		int queueLoop() {
			while (true) {
				std::function<void()> job;
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					condition.wait(lock, [this] { return status < 0 || !jobQueue.empty() || !status; });
					if (status < 0 || (jobQueue.empty() && !status))
						break;
					job = std::move(jobQueue.front());
					jobQueue.pop();
				}
				job();
				in++;
			}
			return in;
		}
		std::thread worker{&Thread::queueLoop, this};

	public:
		~Thread() {
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				status = 0;
			}
			condition.notify_all();
			if (worker.joinable()) {
				worker.join();
			}
			//std::cout<<in<<std::endl;
		}

		void addJob(std::function<void()> function) {
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				jobQueue.push(std::move(function));
			}
			condition.notify_one();
		}

	};

}


#endif //STBOX_THREAD_HH
