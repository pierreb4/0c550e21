double now();

struct State {
  vImage vimg;
  int depth;
  bool isvec;
  State() {}
  State(vImage_ vimg_, bool isvec_, int depth_) : vimg(vimg_), depth(depth_), isvec(isvec_) {}
  ull hash() const {
    ull r = isvec;
    for (Image_ img : vimg) {
      r += hashImage(img)*123413491;
    }
    return r;
  }
};


#include "efficient.hpp"


struct Functions {
  set<int> list;
  vector<int> cost;
  vector<string> name;
  vector<function<bool(const State&,State&)>> f_list;

  void add(const string& name, int cost, const function<bool(const State&,State&)>&func, int list_it);
  void add(string name, int cost, const function<Image(Image_)>&f, int list_it = 1);
  void add(string name, int cost, const function<vImage(Image_)>&f, int list_it = 1);
  void add(string name, int cost, const function<Image(vImage_)>&f, int list_it = 1);
  void add(string name, int cost, const function<vImage(vImage_)>&f, int list_it = 1);
  void add(const vector<point>&sizes, string name, int cost, const function<Image(Image_,Image_)>&f, int list_it = 1);
  string getName(int fi);
  int findfi(string name);
};

struct Depth {
  Functions func;
  unordered_map<int, double> score_map;
  vector<pair<int, double>> score;
  string getName(int fi) {
    return func.getName(fi);
  }
  int findfi(string name) {
    return func.findfi(name);
  }
};

struct DAG {
  vector<Depth> depth;
  TinyNodeBank tiny_node;
  int givens;
  point target_size;
  TinyHashMap hashi;
//  vector<int> binary;
  int add(const State&nxt, bool force = false);
  Image getImg(int nodei) {
    return tiny_node.getImg(nodei); 
  }
  int getChild(int nodei, int fi) {
    return tiny_node.getChild(nodei, fi);
  }
  int getChildFi(int nodei, int fi) {
    return tiny_node.getChildFi(nodei, fi);
  }
  int getPfiSize(int nodei) {
    return tiny_node.getPfiSize(nodei);
  }
  int getPfi(int nodei, int i) {
    return tiny_node.getPfi(nodei, i);
  }
  void build(int DEPTH);
  void benchmark(int DEPTH);
  void initial(Image_ test_in, const vector<pair<Image,Image>>&train, vector<point> sizes, int ti);
  int applyFunc(int DEPTH, int curi, int fi, const State&state);
  int applyFunc(int DEPTH, int curi, int fi);
  void applyFunc(int DEPTH, string name, bool vec);
  void applyFuncs(int DEPTH, vector<pair<string,int>> names, bool vec);
};


struct Pieces;
void brutePieces2(Pieces& pieces, Image_ test_in, const vector<pair<Image, Image>>& train, vector<point> out_sizes);
