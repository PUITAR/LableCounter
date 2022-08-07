/*
 * @author: puitar
 * @Description: 
 * @Date: 2022-08-06 17:08:28
 */

#include "lable_db_engine.h"


using namespace rocksdb;



LableDbEngine::~LableDbEngine() {
    system("rm -rf db/*");
}


LableDbEngine::LableDbEngine(const std::string &db_path) : 
db_path_(db_path), total_timer_(), io_timer_(), graph_size_(0), thread_number_(1)
{
    db_ = nullptr;
    thread_pool_ = nullptr;
    lable_counter_ = nullptr;
    engine_status_ = EngineStatus::rest;

    open_options_.IncreaseParallelism();
    open_options_.create_if_missing = true;
    read_options_ = rocksdb::ReadOptions();
    write_options_ = rocksdb::WriteOptions();
    assert(OpenDb());
}



bool LableDbEngine::OpenDb() {
    Status s = DB::Open(open_options_, db_path_, &db_);
    return s.ok();
}


void LableDbEngine::CloseDb() {
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
    if (thread_pool_) {
        delete[] thread_pool_;
        thread_pool_ = nullptr;
    }
    if (lable_counter_) {
        for (int i = 0; i < graph_size_; i++) {
            delete[] lable_counter_[i];
        }
        delete[] lable_counter_;
        lable_counter_ = nullptr;
    }
}


bool LableDbEngine::BuildGraph(const std::string &txt_path) {
    assert(graph_size_ == 0);
    std::cout << "Building graph...";
    std::ifstream fin(txt_path);
    uint32_t key, value;
    bool success = true;
    int max_id = -1;
    while(fin >> key >> value) {
        int tmp = key>value?key:value;
        max_id = max_id > tmp ? max_id : tmp;
        success &= AddTheOneToNeighbors(key, value);
        success &= AddTheOneToNeighbors(value, key);
    }
    if (success) {
        std::cout << "[success]\n";
        graph_size_ = max_id+1;
    }
    else
        std::cout << "[failed]\n";
    return success;
}


void LableDbEngine::StringToVector(const std::string &str, std::vector<uint32_t> *vec)
{
    size_t len = str.length();
    vec->assign((uint32_t*)str.c_str(), (uint32_t*)str.c_str()+len/sizeof(uint32_t));
}


bool LableDbEngine::Get(uint32_t key, std::string &value) {
    std::string data;
    Status s;
    if (thread_number_ == 1 && engine_status_ == work) {
        io_timer_.StartTimer();
        s = db_->Get(read_options_, std::to_string(key), &data);
        io_timer_.StopTimer();
    }
    else 
        s = db_->Get(read_options_, std::to_string(key), &data);

    if (s.IsNotFound()) 
        return false;
    value = data;
    return true;
}


bool LableDbEngine::Put(uint32_t key, std::string &value) {
    Status s = db_->Put(write_options_, std::to_string(key), Slice(value));
    return s.ok();
}



void LableDbEngine::GetOneHopNeighbors(uint32_t key, std::vector<uint32_t> &neighbors)
{
    std::string neighbors_string;
    Get(key, neighbors_string);
    StringToVector(neighbors_string, &neighbors);
}


void LableDbEngine::GetTwoHopNeighbors(uint32_t key, std::set<uint32_t> &neighbors)
{
    std::vector<uint32_t> one_hop_neighbors;
    GetOneHopNeighbors(key, one_hop_neighbors);
    for (auto one_hop_neighbor: one_hop_neighbors) {
        neighbors.insert(one_hop_neighbor);
        std::vector<uint32_t> two_hop_neighbors;
        GetOneHopNeighbors(one_hop_neighbor, two_hop_neighbors);
        for (auto two_hop_neighbor: two_hop_neighbors)
            neighbors.insert(two_hop_neighbor);
    }
}



bool LableDbEngine::AddTheOneToNeighbors(uint32_t key, uint32_t value)
{
    std::string neighbors_string;
    std::string value_string;
    for (int offset = 0; offset < sizeof(value); offset++) {
        char c = *((char*)&value+offset);
        value_string.push_back(c);
    }
    Get(key, neighbors_string);
    neighbors_string += value_string;
    return Put(key, neighbors_string);
}


// void LableDbEngine::UpdateGraphSize() {
//     if (db_) {
//         uint64_t n;
//         assert(db_->GetIntProperty("rocksdb.estimate-num-keys", &n));
//         graph_size_ = n;
//         return;
//     }
//     graph_size_ = 0;
// }


void LableDbEngine::LoadLables(const std::string &lable_path) {
    assert(graph_size_);

    std::cout << "Loading lables...";
    lables_.resize(graph_size_);
    std::ifstream fin(lable_path);
    int vertex_id, lable_number;
    char lable;
    lable_number_ = -1;
    while (fin >> vertex_id) {
        fin >> lable_number;
        lable_number_ = lable_number_>lable_number?lable_number_:lable_number;
        while(lable_number--) {
            fin >> lable;
            lables_[vertex_id].push_back(lable);
        }
    }
    std::cout << "[success]\n";
}


bool LableDbEngine::GenerateLables(const std::string &lable_path, const std::vector<float> &lable_ratioes)
{
    using namespace std;
    cout << "vertex number: " << graph_size_ << endl;
    cout << "Lable Generating...";

    if (graph_size_ == 0) {
        cout << "[failed]\n";
        return false;
    }

    int lable_number = lable_ratioes.size();
    lable_number_ = lable_number;
    ofstream fout(lable_path);
    int rand_sum = 100000;
    vector<int> rand_max(lable_number);
    for (char lable = 0; lable < lable_number; lable++)
    {
        rand_max[lable] = rand_sum*lable_ratioes[lable];
    }

    for (int v = 0; v < graph_size_; v++)
    {
        vector<char> v_lables;
        for (char lable = 0; lable < lable_number; lable++)
        {
            int rand_result = rand() % rand_sum;
            if (rand_result < rand_max[lable])
                v_lables.push_back(lable);
        }
        fout << v << " " << v_lables.size() << " ";
        for (char lable: v_lables)
            fout << lable << " ";
        fout << "\n"; 
    }
    cout << "[success]\n";
    return true;
}


void LableDbEngine::SetThreadNum(int thread_number)
{
    assert(thread_number >= 1);
    if (graph_size_/thread_number)
        thread_number_ = thread_number;
    else {
        std::cout << "WARN: the thread number is no allowed (thread number = 1)\n";
        thread_number_ = 1;
    }
}



void LableDbEngine::ThreadTask(int start_vertex_id, LableDbEngine* ldbe_ptr)
{
    using namespace std;
    assert(ldbe_ptr->lable_counter_);
    int end_vertex_id = 0;
    if (ldbe_ptr->thread_number_ == 1)
        end_vertex_id = ldbe_ptr->graph_size_;
    else {
        end_vertex_id = start_vertex_id + 
                        ldbe_ptr->graph_size_ / ldbe_ptr->thread_number_;
        end_vertex_id = end_vertex_id < ldbe_ptr->graph_size_?
                        end_vertex_id : ldbe_ptr->graph_size_;
    }
    
    for (int vertex_id = start_vertex_id; vertex_id < end_vertex_id; vertex_id++)
    {
        std::set<uint32_t> two_hop_neighbors;
        ldbe_ptr->GetTwoHopNeighbors(vertex_id, two_hop_neighbors);
        // {
        //     cout << vertex_id << " 2f : ";
        //     for (auto each_neighbor: two_hop_neighbors)
        //         cout << each_neighbor << " ";
        //     cout << endl;
        // }
        for (auto each_neighbor: two_hop_neighbors) {
            for (char lable = 0; lable < ldbe_ptr->lable_number_; lable++) {
                if (ldbe_ptr->HaveTheLable(each_neighbor, lable))
                    ldbe_ptr->lable_counter_[vertex_id][lable]++;
            }
        }
    }
}


bool LableDbEngine::HaveTheLable(int vertex_id, char lable) {
    for (char v_lable: lables_[vertex_id])
        if (v_lable == lable)
            return true;
    return false;
}


void LableDbEngine::TaskSet() {
    assert(engine_status_ == EngineStatus::rest);

    assert(thread_pool_ = new std::thread[thread_number_]);

    if (lable_counter_ == nullptr) {
        assert(lable_counter_ = new int*[graph_size_]);
        for (int i = 0; i < graph_size_; i++)
            assert(lable_counter_[i] = new int[lable_number_]);
    }
    
    for (int i = 0; i < graph_size_; i++)
        for (int j = 0; j < lable_number_; j++)
            lable_counter_[i][j] = 0;
}


void LableDbEngine::Go(int thread_number) {
    SetThreadNum(thread_number);
    TaskSet();

    assert(thread_pool_);
    assert(lable_counter_);
    assert(engine_status_ == EngineStatus::rest);

    engine_status_ = EngineStatus::work;
    total_timer_.StartTimer();

    int interval = graph_size_/thread_number_;

    for (int tid = 0; tid < thread_number_; tid++) {
        thread_pool_[tid] = std::thread(ThreadTask, tid*interval, this);
    }
    for (int tid = 0; tid < thread_number_; tid++)
        thread_pool_[tid].join();

    total_timer_.StopTimer();
    engine_status_ = EngineStatus::finish;
}



void LableDbEngine::ShowTimer() {
    using namespace std;
    assert(engine_status_ == EngineStatus::finish);
    cout << "/========== Timer ==========/" << endl;
    cout << "thread_number: " << thread_number_ << endl;
    cout << "vertex_number: " << graph_size_ << endl;
    cout << "computation_time: " << total_timer_.GetTimerCount() << "ms" << endl;
    if (thread_number_ == 1) {
        cout << "io_time: " << io_timer_.GetTimerCount() << "ms" << endl;
        printf("io_time/computation_time: %.2f%\n", 
               io_timer_.GetTimerCount()/total_timer_.GetTimerCount()*100);
    }
    cout << "\n\n\n";
}


void LableDbEngine::ShowNeighbors() {
    using namespace std;
    cout << "vertex number: " << graph_size_ << endl;
    cout << "one hop neighbors: " << endl;
    for (int v = 0; v < graph_size_; v++) {
        cout << v << " : ";
        vector<uint32_t> one_hop_neighbors;
        GetOneHopNeighbors(v, one_hop_neighbors);
        for (auto one_hop_neighbor: one_hop_neighbors)
            cout << one_hop_neighbor << " ";
        cout << endl;
    }
}


void LableDbEngine::ShowCounter() {
    using namespace std;

    if (lable_counter_) {
        cout << "lable counter:\n"; 
        for (int v = 0; v < graph_size_; v++) {
            cout << v << " : ";
            for (char lable = 0; lable < lable_number_; lable++) {
                cout << lable_counter_[v][lable] << " ";
            }
            cout << "\n";
        }
    }
    else {
        cout << "ERROR: no counter id set\n";
    }
}



void LableDbEngine::ShowLables() {
    using namespace std;
    cout << "lable number: " << lable_number_ << endl;
    cout << "vertex lable: \n";
    for (int v = 0; v < graph_size_; v++) {
        cout << v << " : ";
        for (char lable: lables_[v])
            cout << (int)lable << " ";
        cout << endl;
    }
}




void LableDbEngine::Reset() {
    engine_status_ = EngineStatus::rest;
    total_timer_.ResetTime();
    io_timer_.ResetTime();

    if (thread_pool_) {
        delete[] thread_pool_;
        SetThreadNum(1);
    }    
}











