#pragma once

#include <condition_variable>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <mutex>
#include <thread>

namespace graph {
namespace algorithms {

template<typename Data, typename Task>
std::vector<typename std::result_of<Task(const Data&, int)>::type> run_in_threads(
    const std::vector<Data>& data,
    const int n_threads,
    Task&& task
) {
    using ReturnType = typename std::result_of<Task(const Data&, int)>::type;

    auto iter = data.begin();
    int batch_size = data.size() / n_threads + 1;
    std::vector<std::thread> workers;
    static_assert(std::is_default_constructible<ReturnType>::value, "ReturnType is not default constructible");
    std::vector<ReturnType> results(data.size());

    for (int thread_id = 0; thread_id < n_threads; ++thread_id) {
        workers.push_back(
            std::thread(
                [&data, &task, &results, batch_size, thread_id]{
                    size_t batch_start = batch_size * thread_id;
                    size_t batch_end = std::min(data.size(), batch_start + batch_size);

                    for (size_t itm_idx = batch_start; itm_idx < batch_end; itm_idx++) {
                        results.data()[itm_idx] = task(data[itm_idx], thread_id);
                    }
                }
            )
        );
    }

    for (auto& worker : workers) {
        worker.join();
    }

    return results;
}

}
}