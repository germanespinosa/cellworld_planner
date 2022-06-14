#include <catch.h>
#include <cell_world.h>


using namespace cell_world;
using namespace std;

//TEST_CASE("weighted_random") {
//    std::vector<float> weights{90,56,4};
//    std::vector<unsigned int> counters(3,0);
//    for (int i=0; i<150000000; i++){
//        counters[Chance::weighted_random(weights)]++;
//    }
//    for (auto c:counters)  cout << c << " ";
//    cout << endl;
//}
//
//
struct C : json_cpp::Json_object {
    Json_object_members(
            Add_optional_member(a);
            Add_optional_member(b);
            )
    string a{};
    int b{};
};

TEST_CASE("") {
    json_cpp::Json_vector<C> vc;
    "[{},{\"b\":1},{\"a\":\"a\"},{}]">>vc;
    CHECK("[{},{\"b\":1},{\"a\":\"a\"},{}]" == vc.to_json());
}