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

struct Score {
  // Set the overall score in dimension[0] - Pierre 20241104
  static constexpr size_t SIZE = 14;
  std::array <double, SIZE> dimension;
  
  Score() {
    dimension.fill(0.0);
  }
  Score(double score) {
    dimension.fill(0.0);
    dimension[0] = score;
  }
  Score(const Score& score) {
    std::copy(std::begin(score.dimension), std::end(score.dimension),
      std::begin(dimension));
  }

  size_t size() const { return dimension.size(); }
  operator double() const { return dimension[0]; }

  inline Score& operator+=(const Score& a) {
    for (int i = 0; i < a.size(); i++)
      this->dimension[i] += a.dimension[i];
    return *this;
  }
};

// inline Score compare(Image_ a, Image_ b) {
//   Score ret(0);
//   double max = a.mask.size();

//   if (a.sz == b.sz)
//     ret.overall = std::inner_product(a.mask.begin(), a.mask.end(), b.mask.begin(),
//       0.0, std::plus<>(), std::equal_to<>());
//   else
//     max = 0;

//   if (max == 0) return 0;
//   else
//     return (a.p == b.p) * (a.sz == b.sz) * ret.overall / max;
// }

// How close from 0 (far) to 1 (close) two numbers are - Pierre 20241104
inline double close(double a, double b) {
  if (a == -b) return (double)(1.0E-6);
  return 1 / ((a + b) * (a + b));
}

inline Score compare(Image_ a, Image_ b) {
  Score ret;
  int d = 0;
  double dimension;
  int max = a.mask.size();


  // for (int i = 0; i < std::min(a.h, b.h); i++) {
  //   for (int j = 0; j < std::min(a.w, b.w); j++) 
  //     ret.overall *= close(a(i, j), b(i, j));
  //   for (int j = std::min(a.w, b.w); j < std::max(a.w, b.w); j++) 
  //     ret.overall *= close(0, b(i, j));
  // }
  // for (int i = std::min(a.h, b.h); i < std::max(a.h, b.h); i++) {
  //   for (int j = 0; j < std::min(a.w, b.w); j++) 
  //     ret.overall *= close(0, b(i, j));
  //   for (int j = std::min(a.w, b.w); j < std::max(a.w, b.w); j++) 
  //     ret.overall *= close(0, 10);
  // }


  ret.dimension[d] = (a.x == b.x) * (a.y == b.y) * (a.w == b.w) * (a.h == b.h);
  
  if (a.sz == b.sz)
    dimension = std::inner_product(a.mask.begin(), a.mask.end(), b.mask.begin(),
      0.0, std::plus<>(), std::equal_to<>());
  else
    max = 0;

  if (max == 0)
    dimension = 0.0;
  else
    dimension /= max;

  ret.dimension[d++] *= dimension;

  double a_w = a.w;
  double b_w = b.w;
  int w = a.w + b.w;

  if (w == 0) {
    a_w = 0.0;
    b_w = 0.0;
  }
  else {
    a_w /= w;
    b_w /= w;
  }
  
  ret.dimension[d++] = 1.0 - abs(a_w - b_w);

  double a_h = a.h;
  double b_h = b.h;
  int h = a.h + b.h;

  if (h == 0) {
    a_h = 0.0;
    b_h = 0.0;
  }
  else {
    a_h /= h;
    b_h /= h;
  }

  ret.dimension[d++] = 1.0 - abs(a_h - b_h);

  for (char c = 0; c <= 10; c++) {
    // Compare number of color c pixels - Pierre 20241105
    double a_cnt = std::count(a.mask.begin(), a.mask.end(), c);
    double b_cnt = std::count(b.mask.begin(), b.mask.end(), c);

    if (a.mask.size() == 0)
      a_cnt = 0.0;
    else
      a_cnt /= a.mask.size();

    if (b.mask.size() == 0)
      b_cnt = 0.0;
    else
      b_cnt /= b.mask.size();

    ret.dimension[d++] = 1.0 - abs(a_cnt - b_cnt);
  }

  return ret;
}

inline bool operator<(Score& a, Score& b) {
  return a.dimension[0] < b.dimension[0];
}

inline bool operator>(Score& a, Score& b) {
  return a.dimension[0] > b.dimension[0];
}

extern int d;

inline bool compareScoreD(const pair<int, Score>& a, const pair<int,Score>& b) {
  return a.second.dimension[d] > b.second.dimension[d];
}

inline bool compareScore(const pair<int, Score>& a, const pair<int,Score>& b) {
  return a.second > b.second;
}

inline bool rCompareScore(const pair<int, Score>& a, const pair<int,Score>& b) {
  return a.second < b.second;
}


struct Depth {
  Functions func;
  unordered_map<int, Score> scoreMap;
  vector<pair<int, Score>> scoreVec;
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
