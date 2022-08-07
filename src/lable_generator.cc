/*
 * @author: puitar
 * @Description: 
 * @Date: 2022-08-07 12:03:17
 */

#include "lable_db_engine.h"
using namespace std;


/**
 * @description: 生成标签，更具lable_config的配置生成
 * @param <int> argc 只能是两个
 * @param <char*> 两个参数不要考虑路径只要名称
 *                argv[1] 参数1是图的txt数据集文件名
 *                argv[2] 参数2是生成的标签的名称
 * @return <*>
 */
int main(int argc, char* argv[]) {
    assert(argc == 3);

    LableDbEngine ldbe("./db/");

    ldbe.BuildGraph(string("data/")+argv[1]);

    // ldbe.ShowNeighbors();
    vector<float> ratio;

    ifstream fin("lable/lable_config");

    int lable_number;
    fin >> lable_number;
    cout << "lable number: " << lable_number << endl;

    while (lable_number--) {
        float f;
        fin >> f;
        assert(0 <= f && f <= 1);
        cout << f << endl;
        ratio.push_back(f);
    }
    
    ldbe.GenerateLables(string("lable/")+argv[2], ratio);

    ldbe.CloseDb();

    return 0;
}