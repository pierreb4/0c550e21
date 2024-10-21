struct Piece3 {
  int memi, depth;
};

struct Pieces {
  vector<DAG> dag;
  vector<Piece3> piece;
  vector<int> mem;
};


void makePieces2(Pieces& pieces, vector<pair<Image, Image>> train, vector<point> out_sizes);