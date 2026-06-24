module;

#include <algorithm>
#include <cmath>
#include <vector>

export module weqeqq.terminal:ratio;

export namespace weqeqq::terminal {

inline std::vector<int> RatioReduce(int total, std::vector<int> ratios,
                                    const std::vector<int>& maximums,
                                    const std::vector<int>& values) {
  for (std::size_t i = 0; i < ratios.size(); ++i)
    if (maximums[i] == 0) ratios[i] = 0;
  int total_ratio = 0;
  for (int r : ratios) total_ratio += r;
  if (total_ratio == 0) return values;
  std::vector<int> result;
  int total_remaining = total;
  for (std::size_t i = 0; i < ratios.size(); ++i) {
    if (ratios[i] && total_ratio > 0) {
      int distributed = std::min(
          maximums[i],
          static_cast<int>(std::lround(static_cast<double>(ratios[i]) *
                                       total_remaining / total_ratio)));
      result.push_back(values[i] - distributed);
      total_remaining -= distributed;
      total_ratio -= ratios[i];
    } else {
      result.push_back(values[i]);
    }
  }
  return result;
}

inline std::vector<int> RatioDistribute(int total, std::vector<int> ratios) {
  int total_ratio = 0;
  for (int r : ratios) total_ratio += r;
  std::vector<int> out;
  int total_remaining = total;
  for (std::size_t i = 0; i < ratios.size(); ++i) {
    int distributed;
    if (total_ratio > 0) {
      distributed = static_cast<int>(std::ceil(static_cast<double>(ratios[i]) *
                                               total_remaining / total_ratio));
    } else {
      distributed = total_remaining;
    }
    out.push_back(distributed);
    total_ratio -= ratios[i];
    total_remaining -= distributed;
  }
  return out;
}

struct RatioEdge {
  int size = -1;
  int ratio = 1;
  int minimum_size = 1;
};

inline std::vector<int> RatioResolve(int total,
                                     const std::vector<RatioEdge>& edges) {
  std::vector<int> sizes(edges.size(), -1);
  for (std::size_t i = 0; i < edges.size(); ++i)
    if (edges[i].size >= 0) sizes[i] = edges[i].size;

  for (;;) {
    std::vector<int> flex;
    for (std::size_t i = 0; i < sizes.size(); ++i)
      if (sizes[i] < 0) flex.push_back(static_cast<int>(i));
    if (flex.empty()) break;

    int remaining = total;
    for (int s : sizes)
      if (s >= 0) remaining -= s;

    if (remaining <= 0) {
      for (int i : flex) sizes[i] = edges[i].minimum_size;
      break;
    }

    int total_ratio = 0;
    for (int i : flex) total_ratio += std::max(1, edges[i].ratio);
    double portion = static_cast<double>(remaining) / total_ratio;

    bool fixed_one = false;
    for (int i : flex) {
      if (portion * std::max(1, edges[i].ratio) <= edges[i].minimum_size) {
        sizes[i] = edges[i].minimum_size;
        fixed_one = true;
        break;
      }
    }
    if (!fixed_one) {
      std::vector<int> ratios;
      for (int i : flex) ratios.push_back(std::max(1, edges[i].ratio));
      std::vector<int> dist = RatioDistribute(remaining, ratios);
      for (std::size_t k = 0; k < flex.size(); ++k) sizes[flex[k]] = dist[k];
      break;
    }
  }

  for (std::size_t i = 0; i < sizes.size(); ++i)
    if (sizes[i] < 0) sizes[i] = edges[i].minimum_size;
  return sizes;
}

}
