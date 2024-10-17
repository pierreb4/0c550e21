struct Candidate {
  vImage imgs;
  vector<int> pis; // Pierre 20241002
  double score;
  int cnt_pieces, sum_depth, max_depth;
  Candidate(vImage_ imgs_, vector<int> pis_, double score_) : imgs(imgs_), pis(pis_), score(score_) {
    cnt_pieces = sum_depth = max_depth = -1;
  }
  Candidate(vImage_ imgs_, vector<int> pis_, int cnt_pieces_, int sum_depth_, int max_depth_) :
    imgs(imgs_), pis(pis_), cnt_pieces(cnt_pieces_), sum_depth(sum_depth_), max_depth(max_depth_) {
    score = -1;
  }
};

inline bool operator<(const Candidate& a, const Candidate& b) {
  return a.score > b.score;
}

inline bool compareScore(const pair<string, double>& a, const pair<string, double>& b) {
  return a.second > b.second;
}

vector<Candidate> composePieces2(Pieces&pieces, vector<pair<Image, Image>> train, vector<point> out_sizes);
vector<Candidate> evaluateCands(set<string> fns, Pieces&pieces, const vector<Candidate>&cands, vector<pair<Image,Image>> train);
// Functions3 getFuncs(Pieces&pieces, const vector<Candidate>&cands);