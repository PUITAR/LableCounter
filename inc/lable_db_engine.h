/*
 * @author: puitar
 * @Description: 用于计算标签的类
 * @Date: 2022-08-06 16:28:47
 */


#ifndef LABLE_DB_ENGINE_H
#define LABLE_DB_ENGINE_H


#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdlib.h>
#include "timer.h"
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"


class LableDbEngine {
public:
    /* 注意线程池指针和计数器指针是堆数组，需要释放空间 */
    // 计时器 
    Timer total_timer_;
    Timer io_timer_;

    // 数据库指针
    rocksdb::DB* db_;

    // 标签
    std::vector<std::vector<char> > lables_;
    int lable_number_;

    // 线程
    int thread_number_;
    std::thread* thread_pool_;

    // 图大小
    int graph_size_;

    // 引擎状态
    enum EngineStatus {work, rest, finish} engine_status_;

    // 标签统计计数器
    int** lable_counter_;

    std::string db_path_;

    rocksdb::Options open_options_;
    rocksdb::ReadOptions read_options_;
    rocksdb::WriteOptions write_options_;


    LableDbEngine();

    LableDbEngine(const std::string &db_path);

    ~LableDbEngine();

    bool OpenDb();

    void CloseDb();

    bool BuildGraph(const std::string &txt_path);

    void GetOneHopNeighbors(uint32_t key, std::vector<uint32_t> &neighbors);

    void GetTwoHopNeighbors(uint32_t key, std::set<uint32_t> &neighbors);

    void StringToVector(const std::string &str, std::vector<uint32_t> *vec); 

    bool Get(uint32_t key, std::string &value);

    bool Put(uint32_t key, std::string &value);

    bool AddTheOneToNeighbors(uint32_t key, uint32_t value);

    // void UpdateGraphSize();

    void LoadLables(const std::string &lable_path);

    bool GenerateLables(const std::string &lable_path, const std::vector<float> &lable_ratioes);

    void SetThreadNum(int thread_number);

    static void ThreadTask(int start_vertex_id, LableDbEngine* ldbe_ptr);

    bool HaveTheLable(int vertex_id, char lable);

    void TaskSet();

    void Go(int thread_number);

    void ShowTimer();

    void ShowNeighbors();

    void ShowCounter();

    void ShowLables();

    void Reset();
};



#endif