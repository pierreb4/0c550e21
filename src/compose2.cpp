#include "precompiled_stl.hpp"
using namespace std;
#include "utils.hpp"
#include "core_functions.hpp"
#include "image_functions.hpp"
#include "image_functions2.hpp"
#include "visu.hpp"

#include "brute2.hpp"
#include "pieces.hpp"
#include "compose2.hpp"

#include "timer.hpp"
#include <map>

/*
struct Candidate {
  vector<Image> imgs;
  double score;
};
*/


extern size_t keep_best, d_size;
extern int MINDEPTH, MAXDEPTH, d, print_times;


struct mybitset {
  vector<ull> data;
  mybitset(int n) {
    data.resize((n+63)/64);
  }
  int operator[](int i) {
    return data[i>>6]>>(i&63)&1;
  }
  void set(int i, ull v) {
    int bit = i&63;
    data[i>>6] &= ~(1ull<<bit);
    data[i>>6] |= (v<<bit);
  }
  ull hash() {
    ull r = 1;
    for (ull h : data) {
      r = r*137139+h;
    }
    return r;
  }
};



int popcount64c(ull x) {
  const uint64_t m1  = 0x5555555555555555; //binary: 0101...
  const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
  const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
  const uint64_t m8  = 0x00ff00ff00ff00ff; //binary:  8 zeros,  8 ones ...
  const uint64_t m16 = 0x0000ffff0000ffff; //binary: 16 zeros, 16 ones ...
  const uint64_t m32 = 0x00000000ffffffff; //binary: 32 zeros, 32 ones
  const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...
  x -= (x >> 1) & m1;             //put count of each 2 bits into those 2 bits
  x = (x & m2) + ((x >> 2) & m2); //put count of each 4 bits into those 4 bits
  x = (x + (x >> 4)) & m4;        //put count of each 8 bits into those 8 bits
  return (x * h01) >> 56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
}

int popcount64d(ull x) {
  int pop = 0;
  while (x) {
    x &= x-1;
    pop++;
  }
  return pop;
}



vector<Candidate> greedyCompose2(Pieces&pieces, vector<Image>&target, vector<point> out_sizes) {
  if (pieces.piece.empty()) return {};

  Timer greedy_fill_time;

  {
    int d = -1;
    for (Piece3&p : pieces.piece) {
      assert(p.depth >= d);
      d = p.depth;
    }
  }

  vector<Image> init;
  vector<int> sz;
  {
    for (int i = 0; i < pieces.dag.size(); i++) {
      if (i < target.size()) assert(out_sizes[i] == target[i].sz);
      init.push_back(core::full(out_sizes[i], 10));
      sz.push_back(init.back().mask.size());
    }
  }

  vector<Candidate> rets;

  int n = pieces.piece.size();

  // // Display DAGs, nodes and children - Pierre 20241021
  // {
  //   cout << __FILE__ << " pieces.dag.size: " << pieces.dag.size() << endl;
  //   for (int i = 0; i < pieces.dag.size(); i++)
  //   {
  //     cout << "Node size: " << pieces.dag[i].tiny_node.size() << endl;
  //     for (int j = 0; j < pieces.dag[i].tiny_node.size(); j++)
  //       for (int k = 0; k < pieces.dag[i].depth[MAXDEPTH/10-1].func.name.size(); k++)
  //         if (pieces.dag[i].tiny_node.node[j].child.fi(k) != TinyChildren::None)
  //         {
  //           cout << "Dag: " << i << " node: " << j;
  //           cout << " num: " << pieces.dag[i].tiny_node.node[j].child.fi(k);
  //           cout << " func: " << pieces.dag[i].depth[MAXDEPTH/10-1].getName(k); 
  //           cout << endl;
  //         }
  //   }
  // }

  int M = 0;
  for (int s : sz) M += s;

  const int M64 = (M+63)/64;
  vector<ull> bad_mem, active_mem;
  vector<int> img_ind, bad_ind, active_ind;

  {
    active_mem.reserve(n*M64);
    bad_mem.reserve(n*M64);
    TinyHashMap seen;
    mybitset badi(M), blacki(M);
    for (int i = 0; i < n; i++) {
      // cout << __FILE__ << " Piece: " << i << " MAXDEPTH: " << MAXDEPTH << endl;
      int x = 0, y = 0;
      for (int j = 0; j < sz.size(); j++) {
	      int*ind = &pieces.mem[pieces.piece[i].memi];
	      Image_ img = pieces.dag[j].getImg(ind[j]);
//        // Add to corresponding func usage counter? No as this seems to be part of selecting the best piece - Pierre 20241002
//        int fi = pieces.dag[j].tiny_node.node[ind[j]].child.fi(0);
//        cout << "  Img: " << j << " fi: " << fi << endl;
//        fis.push_back(fi);
	      const vector<char>&p = img.mask;
	      const vector<char>&t = j < target.size() ? target[j].mask : init[j].mask;

        if (p.size() != sz[j]) {
          cout << __FILE__ << " piece: " << i << " dag: " << j << " img.sz: " << img.sz.x << " " << img.sz.y << endl;
          cout << __FILE__ << " piece: " << i << " dag: " << j << " img.x: " << img.x << " img.y: " << img.y << endl;
          cout << __FILE__ << " piece: " << i << " dag: " << j << " img.w: " << img.w << " img.h: " << img.h << endl;
          cout << __FILE__ << " piece: " << i << " dag: " << j << " img.mask.size: " << img.mask.size() << endl;
          cout << __FILE__ << " piece: " << i << " dag: " << j << " img.mask: " << endl;
          // for (int i = 0; i < img.h; i++) {
          //   for (int j = 0; j < img.w; j++) {
          //     cout << img(i,j);
          //   }
          //   cout << endl;
          // }
          // cout << endl;

          cout << __FILE__ << "  j: " << j << " sz[j]: " << sz[j] << " p.size: " << p.size() << " t.size: " << t.size() << endl;
        }

	      assert(p.size() == sz[j]);
	      assert(t.size() == sz[j]);
	      for (int k = 0; k < sz[j]; k++) {
	        badi.set  (x++, (p[k] != t[k]));
	        blacki.set(y++, (p[k] == 0));
	      }
      }
      img_ind.push_back(i);

      active_ind.push_back(active_mem.size());
      for (ull v : blacki.data)
	      active_mem.push_back(v);

      bad_ind.push_back(bad_mem.size());
      for (ull v : badi.data)
	      bad_mem.push_back(v);
    }
  }

  //cout << active_mem.size() << endl;
  //cout << bad_mem.size() << endl;
  //exit(0);

  int max_piece_depth = 0;
  for (int i = 0; i < n; i++)
    max_piece_depth = max(max_piece_depth, pieces.piece[i].depth);

  //mt19937 mrand(0);



  auto greedyComposeCore = [&](mybitset&cur, const mybitset&careMask, const int piece_depth_thres, vImage&ret, vector<int>&pis) {
    vector<int> sparsej;
    for (int j = 0; j < M64; j++) {
      if (~cur.data[j] & careMask.data[j]) sparsej.push_back(j);
    }

    vector<ull> best_active(M64), tmp_active(M64);
    int besti = -1;
    pair<int,int> bestcnt = {0,0};
    //for (int i : order) {
    //  if (skip[i] || pieces.piece[img_ind[i]].depth > piece_depth_thres) continue;
    for (int i = 0; i < img_ind.size(); i++) {
      if (pieces.piece[img_ind[i]].depth > piece_depth_thres) continue;

      for (int k : {0,1,2}) {
	      const ull*active_data = &active_mem[active_ind[i]];

	      ull flip = (k == 0 ? ~0 : 0);
	      ull full = (k == 2 ? ~0 : 0);

	      const ull*bad_data = &bad_mem[bad_ind[i]];
	      int cnt = 0, covered = 0, ok = 1;
	      for (int j = 0; j < M64; j++) {
	        ull active = ((active_data[j]^flip) | full);
	        if (~cur.data[j] & bad_data[j] & active) {
	          ok = 0;
	          break;
	        }
	      }
	      if (!ok) continue;
	      for (int j : sparsej) {
	        ull active = ((active_data[j]^flip) | full);
	        cnt += popcount64d(active & ~cur.data[j] & careMask.data[j]);
	        //covered += popcount64d(bad.data[j] & cur.data[j] & careMask.data[j]);
	      }
	      if (ok && make_pair(cnt,-covered) > bestcnt) {
	        bestcnt = make_pair(cnt,-covered);
	        besti = i;

	        if (k == 0) {
	          for (int j = 0; j < M64; j++)
	            tmp_active[j] = ~active_data[j];
	        } else if (k == 1) {
	          for (int j = 0; j < M64; j++)
	            tmp_active[j] = active_data[j];
	        } else {
	          for (int j = 0; j < M64; j++)
	            tmp_active[j] = ~0;
	        }
	        best_active = tmp_active;
	      }
      }
    }

    if (besti == -1) return -1;

    {
      int i = img_ind[besti], x = 0;

      int depth = pieces.piece[i].depth;

      // Print pieces - Pierre 202451016
      // cout << "Best piece: " << i << " depth: " << depth << endl;
      pis.push_back(i);

      for (int l = 0; l < ret.size(); l++) {
        int*ind = &pieces.mem[pieces.piece[i].memi];
        const vector<char>&mask = pieces.dag[l].getImg(ind[l]).mask;

        // Print pieces - Pierre 20241016
        // cout << " Piece: " << l;

        // for (int j = 0; j < pieces.dag[l].getPfiSize(ind[l]); j++) {
        //   int pfi = pieces.dag[l].getPfi(ind[l], j);
        //   if (pfi != TinyChildren::None)
        //     cout << "\tFi: " << pfi;
        // }
        // cout << endl;
        
	      for (int j = 0; j < sz[l]; j++) {
	        if ((best_active[x>>6]>>(x&63)&1) && ret[l].mask[j] == 10) {
	          //assert(cur[x-1] == 0);
	          ret[l].mask[j] = mask[j];
	        }

	        x++;
	      }
      }

      for (int j = 0; j < M; j++) {
	      if (best_active[j>>6]>>(j&63)&1) cur.set(j, 1);
      }

      return depth;
    }
  };









  map<ull,Image> greedy_fill_mem;

  // Try more - Pierre 20241107
  // int maxiters = 10;
  int maxiters = 16 * MAXDEPTH / 10;

  //vector<int> skip(img_ind.size());
  //vector<int> order;
  //for (int i = 0; i < img_ind.size(); i++) order.push_back(i);

  for (int pdt = max_piece_depth%10; pdt <= max_piece_depth; pdt += 10) {
    int piece_depth_thres = pdt;

    for (int it0 = 0; it0 < 10; it0++) {
      for (int mask = 1; mask < min(1<<target.size(), 1<<5); mask++) {
	      vector<int> maskv;
	      for (int j = 0; j < target.size(); j++)
	      if (mask>>j&1) maskv.push_back(j);

	      int caremask;
	      if (it0 < maskv.size()) {
	        caremask = 1<<maskv[it0];
	      } else {
	        continue;
	        /*caremask = 1<<maskv[mrand()%maskv.size()];
	        for (int i = 0; i < n; i++)
	        skip[i] = mrand()%3;
	        random_shuffle(order.begin(), order.end());*/
	      }

	      mybitset cur(M), careMask(M);
	      {
	        int base = 0;
	        for (int j = 0; j < sz.size(); j++) {
	          if (!(mask>>j&1))
	            for (int k = 0; k < sz[j]; k++)
		        cur.set(base+k, 1);
	          if ((caremask>>j&1))
	            for (int k = 0; k < sz[j]; k++)
		        careMask.set(base+k, 1);
	          base += sz[j];
	        }
	      }


	      int cnt_pieces = 0;
	      vector<int> piece_depths;
	      int sum_depth = 0, max_depth = 0;

	      vector<Image> ret = init;
        vector<int> pis;
	      for (int it = 0; it < maxiters; it++) {

	        int depth = greedyComposeCore(cur, careMask, piece_depth_thres, ret, pis);
	        if (depth == -1) break;
	        piece_depths.push_back(depth);
	        cnt_pieces++;
	        sum_depth += depth;
	        max_depth = max(max_depth, depth);


	        {
	          greedy_fill_time.start();
	          vImage cp = ret;
	          int carei = 31-__builtin_clz(caremask);
	          assert(caremask == 1<<carei);
	          int ok = 1;
	          {
	            Image& img = cp[carei];
	            for (char&c : img.mask) if (c == 10) c = 0;
	            img = greedyFillBlack(img);
	            if (img != target[carei]) ok = 0;
	          }
	          if (ok) {
	            for (int i = 0; i < cp.size(); i++) {
		            if (i == carei) continue;
		            Image& img = cp[i];
		            for (char&c : img.mask) if (c == 10) c = 0;
		            ull h = hashImage(img);
		            if (!greedy_fill_mem.count(h)) {
		              greedy_fill_mem[h] = greedyFillBlack(img);
		            }
		            img = greedy_fill_mem[h];
		            if (img.w*img.h <= 0) ok = 0;
	            }
	            if (ok) {
                // cout << "GC2 cp  size: " <<  cp.size() << endl;
                // cout << "GC2 pis size: " << pis.size() << endl;
		            rets.emplace_back(cp, pis, cnt_pieces+1, sum_depth, max_depth);
              }
	          }
	          greedy_fill_time.stop();
	        }
	      }

	      /*for (Image&img : ret)
	        for (char&c : img.mask)
	        if (c == 10) c = 0;
	      */

        // cout << "GC2 ret size: " << ret.size() << endl;
        // cout << "GC2 pis size: " << pis.size() << endl;
	      rets.emplace_back(ret, pis, cnt_pieces, sum_depth, max_depth);
      }
    }
  }

  if (print_times)
    greedy_fill_time.print("Greedy fill time");

  return rets;
}



vector<Candidate> composePieces2(Pieces&pieces, vector<pair<Image, Image>> train, vector<point> out_sizes) {
  vector<Candidate> cands;

  vector<Image> target;
  for (auto [in,out] : train)
    target.push_back(out);

  for (const Candidate&cand : greedyCompose2(pieces, target, out_sizes)) {
    cands.push_back(cand);
  }
  return cands;
}


vector<Candidate> evaluateCands(Pieces&pieces, const vector<Candidate>&cands, vector<pair<Image,Image>> train) {
  vector<Candidate> ret;
  for (const Candidate& cand : cands) {
    vImage_ imgs = cand.imgs;
    vector<int> pis = cand.pis;

    if (cand.max_depth >= 200) {
      cout << __FILE__ << " cand.max_depth: " << cand.max_depth << endl;
    }
    
    assert(cand.max_depth >= 0 && cand.max_depth < 200);
    // assert(cand.max_depth >= 0 && cand.max_depth < 100);
    assert(cand.cnt_pieces >= 0 && cand.cnt_pieces < 100);
    
    // cout << __FILE__ << " Max_depth: " << cand.max_depth << " Cnt_pieces: " << cand.cnt_pieces << endl;
    
    double prior = cand.max_depth + cand.cnt_pieces * 1e-3; //cnt_pieces;

    // cout << "- Piece numbers: ";
    // for (int pi : pis) cout << pi << ' ';
    // cout << endl;

    // cout << "  - " << __FILE__ << " Cands.size: " << cands.size() << endl;
    // cout << "  - " << __FILE__ << " Train.size: " << train.size() << endl;

    // Finetune comparison - Pierre 20241030
    // int goods = 0;
    Score goods;
    for (int i = 0; i < train.size(); i++) {
      // cout << __FILE__ << ":" << __LINE__ << " Goods.dimension[0] before: " 
      //   << goods.dimension[0] << endl;

      // goods += (imgs[i] == train[i].second);
      goods += compare(imgs[i], train[i].second);

      // cout << __FILE__ << ":" << __LINE__ << " Goods.dimension[0] after: " 
      //   << goods.dimension[0] << endl;
    }
    
    // Set to just goods but no difference initially - Pierre 20241101
    // Score score = goods - prior * 0.0001;
    Score score(goods);

    for (d = 0; d < Score::SIZE; d++) {
      // score.dimension[d] -= prior * 0.0001;

      cout << __FILE__ << ":" << __LINE__ << " goods[" << d << "]: " << goods.dimension[d] << endl;
      cout << __FILE__ << ":" << __LINE__ << " score[" << d << "]: " << score.dimension[d] << endl;
    }


    cout << __FILE__ << ":" << __LINE__ << " cand.pis.size: " << cand.pis.size() << endl;

    // Collect functions - Pierre 20241018
    for (int pi : cand.pis) {
      int* ind = &pieces.mem[pieces.piece[pi].memi];
      for (int j = 0; j < pieces.dag.size(); j++) {
        int pfis = pieces.dag[j].getPfiSize(ind[j]);
        for (int k = 0; k < pfis; k++) {
          int pfi = pieces.dag[j].getPfi(ind[j], k);
          for (int DEPTH = MINDEPTH; DEPTH <= MAXDEPTH; DEPTH += 10) {
            if (pfi != TinyChildren::None) {
              // Store scores in pieces.dag[j] - Pierre 20241018
              auto it = pieces.dag[j].depth[DEPTH / 10 - 1].scoreMap.find(pfi);
              if (it != pieces.dag[j].depth[DEPTH / 10 - 1].scoreMap.end()) {
                // Update fn score if needed
                if (it->second < score) {
                  it->second = std::move(score);
                }
              }
              else {
                // Insert fn with score
                cout << __FILE__ << ":" << __LINE__ << " Inserting score" << endl;
                pieces.dag[j].depth[DEPTH / 10 - 1].scoreMap[pfi] = std::move(score);
              }
            }
          }
        }
      }
    }

    // Pierre 20240926
    cout << "  max_d: " << cand.max_depth;
    cout << "\tpiece: " << cand.cnt_pieces;
    cout << "\tprior: " << prior;
    cout << "\tgoods: " << goods;
    cout << "\tscore: " << score << endl;

    Image answer = imgs.back();
    if (answer.w > 30 || answer.h > 30 || answer.w * answer.h == 0) {
      goods.dimension[0] = 0;
      // cout << __FILE__ << " bad size" << endl;
    }
    for (int i = 0; i < answer.h; i++)
      for (int j = 0; j < answer.w; j++)
        if (answer(i, j) < 0 || answer(i, j) >= 10) {
          goods.dimension[0] = 0;
          // cout << __FILE__ << " bad value" << endl;
        }
    if (goods.dimension[0])
      ret.emplace_back(imgs, pis, score.dimension[0]);
  }

  cout << __FILE__ << ":" << __LINE__ << " Done with cands" << endl;

  for (int DEPTH = MINDEPTH; DEPTH <= MAXDEPTH; DEPTH += 10) {
    // Store and sort scores in pieces.dag[j] - Pierre 20241018
    for (int j = 0; j < pieces.dag.size(); j++) {
      Depth& depth = pieces.dag[j].depth[DEPTH / 10 - 1];
      depth.scoreVec.clear();

      // cout << __FILE__ << ":" << __LINE__ << " map.size: " << depth.scoreMap.size() << endl;

      depth.scoreVec.reserve(depth.scoreMap.size());
      copy(depth.scoreMap.begin(), depth.scoreMap.end(), back_inserter(depth.scoreVec));

      // cout << __FILE__ << ":" << __LINE__ << " Copied map" << endl;

      // Sort and keep only a number of best scores for each dimension - Pierre 20241104
      for (d = 0; d < Score::SIZE; d++) {

        // cout << __FILE__ << ":" << __LINE__ << " depth.scoreVec: " 
        //   << depth.scoreVec.size() << endl;

        if (depth.scoreVec.begin() + d * keep_best < depth.scoreVec.end()) {
          sort(depth.scoreVec.begin() + d * keep_best, depth.scoreVec.end(), compareScoreD);
        }
      }

      // cout << __FILE__ << ":" << __LINE__ << " Size before: " << depth.scoreVec.size() << endl;
      // Not sure that this maade sense - Pierre 20241107
      // depth.scoreVec.resize(std::min(depth.scoreVec.size(), Score::SIZE * keep_best * DEPTH / 10));        
      depth.scoreVec.resize(std::min(depth.scoreVec.size(), Score::SIZE * keep_best));        
      // cout << __FILE__ << ":" << __LINE__ << " Size after: " << depth.scoreVec.size() << endl;

      // for (const auto& s : pieces.dag[j].scores) {
      //   cout << __FILE__ << " DAG[" << j << "]: " << s.first << " " << s.second << endl;
      // }
      // cout << endl;
    }
    cout << __FILE__ << ":" << __LINE__ << " DEPTH: " << DEPTH << endl;
  }

  cout << __FILE__ << ":" << __LINE__ << " ret.size: " << ret.size() << endl;

  sort(ret.begin(), ret.end());
  //printf("%.20f\n\n", ret[0].score);
  return ret;
}
