//#include <ext/pb_ds/assoc_container.hpp>
//using namespace __gnu_pbds;

double now();

struct State {
  vImage vimg;
  vector <int> pfi;
  int depth;
  bool isvec;
  State() {}
  State(vImage_ vimg_, int pfi_, bool isvec_, int depth_) : vimg(vimg_), isvec(isvec_), depth(depth_) {
    int pfis = pfi.size();
    if(pfis+1 > 0) {
      pfi.resize(pfis+1);
      pfi[pfis] = pfi_;
    }
  }
  ull hash() const {
    ull r = isvec;
    for (Image_ img : vimg) {
      r += hashImage(img)*123413491;
    }
    return r;
  }
};


#include "efficient.hpp"


// Unused - Pierre 20241009
// struct Node {
//   State state;
//   //vImage vimg;
//   //bool isvec;
//   vector<pair<int,int>> child;
//   //int depth;
//   int par, pfi;
//   bool freed, ispiece;
//   Node() {
//     freed = ispiece = false;
//   }
//   Node(vImage_ vimg_, bool isvec_, int depth_, int par_ = -1, int pfi_ = -1) : par(par_), pfi(pfi_) {
//     freed = ispiece = false;
//     state.vimg = vimg_;
//     state.isvec = isvec_;
//     state.depth = depth_;
//   }
// };


struct Functions3 {
  vector<int> listed, cost;
  vector<string> names;
  vector<function<bool(const State&,State&)>> f_list;

  void add(const string& name, int cost, const function<bool(const State&,State&)>&func, int list);
  void add(string name, int cost, const function<Image(Image_)>&f, int list = 1);
  void add(string name, int cost, const function<vImage(Image_)>&f, int list = 1);
  void add(string name, int cost, const function<Image(vImage_)>&f, int list = 1);
  void add(string name, int cost, const function<vImage(vImage_)>&f, int list = 1);
  void add(const vector<point>&sizes, string name, int cost, const function<Image(Image_,Image_)>&f, int list = 1);
  string getName(int fi);
  int findfi(string name);
};


struct DAG {
  Functions3 funcs;
  //vector<Node> node;
  TinyNodeBank tiny_node;
  int givens;
  point target_size;
  //gp_hash_table<ull, int> hashi;
  TinyHashMap hashi;
  vector<int> binary;
  int add(const State&nxt, bool force = false);
  Image getImg(int nodei);
  int getPfiSize(int nodei) {
    return tiny_node.getPfiSize(nodei);
  }
  int getPfi(int nodei, int i) {
    return tiny_node.getPfi(nodei, i);
  }
  void build();
  void buildBinary();
  void initial(Image_ test_in, const vector<pair<Image,Image>>&train, vector<point> sizes, int ti);
  void benchmark();
  int applyFunc(int curi, int fi, const State&state);
  int applyFunc(int curi, int fi);
  void applyFunc(string name, bool vec);
  void applyFuncs(vector<pair<string,int>> names, bool vec);
};


struct Pieces;
vector<DAG> brutePieces2(Image_ test_in, const vector<pair<Image,Image>>&train, vector<point> out_sizes);
