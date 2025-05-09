#include <mr-contractor/contractor.hpp>
#include <cstddef>
#include <map>

using TaskMap = std::map<size_t, mr::Task<int>>;

inline auto create_nested_task_map() -> TaskMap {
  static TaskMap res;

  static auto prot1 = mr::Sequence { [](int) -> int { return 47; } };
  res[1] = mr::apply(prot1, 0);

  static auto prot2 = mr::Sequence { std::ref(prot1), std::ref(prot1) };
  res[2] = mr::apply(prot2, 0);

  static auto prot4 = mr::Sequence { std::ref(prot2), std::ref(prot2) };
  res[4] = mr::apply(prot4, 0);

  static auto prot8 = mr::Sequence { std::ref(prot4), std::ref(prot4) };
  res[8] = mr::apply(prot8, 0);

  static auto prot16 = mr::Sequence { std::ref(prot8), std::ref(prot8) };
  res[16] = mr::apply(prot16, 0);

  static auto prot32 = mr::Sequence { std::ref(prot16), std::ref(prot16) };
  res[32] = mr::apply(prot32, 0);

  static auto prot64 = mr::Sequence { std::ref(prot32), std::ref(prot32) };
  res[64] = mr::apply(prot64, 0);

  static auto prot128 = mr::Sequence { std::ref(prot64), std::ref(prot64) };
  res[128] = mr::apply(prot128, 0);

  return std::move(res);
}

inline auto create_flat_task_map() -> TaskMap {
  constexpr int size = 128;

  TaskMap res;

  static std::vector<std::function<int(int)>> funcs;
  funcs.resize(size);

  for (int i = 0; i < size; i++) {
    funcs[i] = [i](int) -> int { return i; };
  }

  static auto prot1 = mr::Sequence {
    funcs[0],
  };
  res[1] = mr::apply(prot1, 0);

  static auto prot2 = mr::Sequence {
    funcs[0],
    funcs[1],
  };
  res[2] = mr::apply(prot2, 0);

  static auto prot4 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
  };
  res[4] = mr::apply(prot4, 0);

  static auto prot8 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
    funcs[4],
    funcs[5],
    funcs[6],
    funcs[7],
  };
  res[8] = mr::apply(prot8, 0);

  static auto prot16 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
    funcs[4],
    funcs[5],
    funcs[6],
    funcs[7],
    funcs[8],
    funcs[9],
    funcs[10],
    funcs[11],
    funcs[12],
    funcs[13],
    funcs[14],
    funcs[15],
  };
  res[16] = mr::apply(prot16, 0);

  static auto prot32 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
    funcs[4],
    funcs[5],
    funcs[6],
    funcs[7],
    funcs[8],
    funcs[9],
    funcs[10],
    funcs[11],
    funcs[12],
    funcs[13],
    funcs[14],
    funcs[15],
    funcs[16],
    funcs[17],
    funcs[18],
    funcs[19],
    funcs[20],
    funcs[21],
    funcs[22],
    funcs[23],
    funcs[24],
    funcs[25],
    funcs[26],
    funcs[27],
    funcs[28],
    funcs[29],
    funcs[30],
    funcs[31],
  };
  res[32] = mr::apply(prot32, 0);

  static auto prot64 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
    funcs[4],
    funcs[5],
    funcs[6],
    funcs[7],
    funcs[8],
    funcs[9],
    funcs[10],
    funcs[11],
    funcs[12],
    funcs[13],
    funcs[14],
    funcs[15],
    funcs[16],
    funcs[17],
    funcs[18],
    funcs[19],
    funcs[20],
    funcs[21],
    funcs[22],
    funcs[23],
    funcs[24],
    funcs[25],
    funcs[26],
    funcs[27],
    funcs[28],
    funcs[29],
    funcs[30],
    funcs[31],
    funcs[32],
    funcs[33],
    funcs[34],
    funcs[35],
    funcs[36],
    funcs[37],
    funcs[38],
    funcs[39],
    funcs[40],
    funcs[41],
    funcs[42],
    funcs[43],
    funcs[44],
    funcs[45],
    funcs[46],
    funcs[47],
    funcs[48],
    funcs[49],
    funcs[50],
    funcs[51],
    funcs[52],
    funcs[53],
    funcs[54],
    funcs[55],
    funcs[56],
    funcs[57],
    funcs[58],
    funcs[59],
    funcs[60],
    funcs[61],
    funcs[62],
    funcs[63],
  };
  res[64] = mr::apply(prot64, 0);

  static auto prot128 = mr::Sequence {
    funcs[0],
    funcs[1],
    funcs[2],
    funcs[3],
    funcs[4],
    funcs[5],
    funcs[6],
    funcs[7],
    funcs[8],
    funcs[9],
    funcs[10],
    funcs[11],
    funcs[12],
    funcs[13],
    funcs[14],
    funcs[15],
    funcs[16],
    funcs[17],
    funcs[18],
    funcs[19],
    funcs[20],
    funcs[21],
    funcs[22],
    funcs[23],
    funcs[24],
    funcs[25],
    funcs[26],
    funcs[27],
    funcs[28],
    funcs[29],
    funcs[30],
    funcs[31],
    funcs[32],
    funcs[33],
    funcs[34],
    funcs[35],
    funcs[36],
    funcs[37],
    funcs[38],
    funcs[39],
    funcs[40],
    funcs[41],
    funcs[42],
    funcs[43],
    funcs[44],
    funcs[45],
    funcs[46],
    funcs[47],
    funcs[48],
    funcs[49],
    funcs[50],
    funcs[51],
    funcs[52],
    funcs[53],
    funcs[54],
    funcs[55],
    funcs[56],
    funcs[57],
    funcs[58],
    funcs[59],
    funcs[60],
    funcs[61],
    funcs[62],
    funcs[63],
    funcs[64],
    funcs[65],
    funcs[66],
    funcs[67],
    funcs[68],
    funcs[69],
    funcs[70],
    funcs[71],
    funcs[72],
    funcs[73],
    funcs[74],
    funcs[75],
    funcs[76],
    funcs[77],
    funcs[78],
    funcs[79],
    funcs[80],
    funcs[81],
    funcs[82],
    funcs[83],
    funcs[84],
    funcs[85],
    funcs[86],
    funcs[87],
    funcs[88],
    funcs[89],
    funcs[90],
    funcs[91],
    funcs[92],
    funcs[93],
    funcs[94],
    funcs[95],
    funcs[96],
    funcs[97],
    funcs[98],
    funcs[99],
    funcs[100],
    funcs[101],
    funcs[102],
    funcs[103],
    funcs[104],
    funcs[105],
    funcs[106],
    funcs[107],
    funcs[108],
    funcs[109],
    funcs[110],
    funcs[111],
    funcs[112],
    funcs[113],
    funcs[114],
    funcs[115],
    funcs[116],
    funcs[117],
    funcs[118],
    funcs[119],
    funcs[120],
    funcs[121],
    funcs[122],
    funcs[123],
    funcs[124],
    funcs[125],
    funcs[126],
    funcs[127],
  };
  res[128] = mr::apply(prot128, 0);

  static_assert(size == 128, "\n\nCome on man, get your shit together\n\n");

  return res;
}
