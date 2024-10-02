struct Candidate {
  vImage imgs;
  vector<int> fis; // Pierre 20241002
  double score;
  int cnt_pieces, sum_depth, max_depth;
  Candidate(vImage_ imgs_, vector<int> fis_, double score_) : imgs(imgs_), fis(fis_), score(score_) {
    cnt_pieces = sum_depth = max_depth = -1;
  }
  Candidate(vImage_ imgs_, vector<int> fis_, int cnt_pieces_, int sum_depth_, int max_depth_) :
    imgs(imgs_), fis(fis_), cnt_pieces(cnt_pieces_), sum_depth(sum_depth_), max_depth(max_depth_) {
    score = -1;
  }
};

inline bool operator<(const Candidate& a, const Candidate& b) {
  return a.score > b.score;
}

vector<Candidate> composePieces2(Pieces&pieces, vector<pair<Image, Image>> train, vector<point> out_sizes);
vector<Candidate> evaluateCands(Pieces&pieces, const vector<Candidate>&cands, vector<pair<Image,Image>> train);
Functions3 getFuncs(Pieces&pieces, const vector<Candidate>&cands);