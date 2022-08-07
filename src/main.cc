/*
 * @author: 
 * @Description: 主函数
 * @Date: 2022-08-07 00:37:03
 */
#include "lable_db_engine.h"

/**
 * @description: 计算程序
 * @param <int> argc 有且只有2个参数
 * @param <char*> 
 *         argv[1] 参数1是图数据txt文件的文件名
 *         argv[2] 参数2是对应图的标签txt文件名（一定要和参数1的图相匹配）
 * @return <*>
 */
int main(int argc, char* argv[]) {
    assert(argc == 3);
    
    LableDbEngine ldbe("db/");
    ldbe.BuildGraph(std::string("data/")+argv[1]);
    ldbe.LoadLables(std::string("lable/")+argv[2]);
    std::cout << "\n\n\n";

    // ldbe.ShowNeighbors();
    // ldbe.ShowLables();
    // ldbe.ShowCounter();

_1_threads:
    ldbe.Go(1);
    ldbe.ShowTimer();
    ldbe.Reset();

_10_threads:
    ldbe.Go(10);
    ldbe.ShowTimer();
    ldbe.Reset();

_20_threads:
    ldbe.Go(20);
    ldbe.ShowTimer();
    ldbe.Reset();

_40_threads:
    ldbe.Go(40);
    ldbe.ShowTimer();
    ldbe.Reset();

    return 0;
}