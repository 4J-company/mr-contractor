#include <filesystem>
#include <fstream>
#include <print>
#include <cmath>

#include <mr-contractor/contractor.hpp>
#include <work_contract/work_contract_group.h>

int main() {
  bcpp::work_contract_group group;
  auto c1 = group.create_contract([]() {});
  c1.schedule();

  group.execute_next_contract();
  group.execute_next_contract();
}

// template <typename Ret, typename... Args>
// struct TaskPrototypeBuilder;
// 
// template <typename Ret, typename... Args>
// auto get_task_prototype() {
//   return std::ref(TaskPrototypeBuilder<Ret, Args...>::create());
// }
// 
// template <typename ResultT, typename ...Args>
// auto make_task(Args ...args) {
//   if constexpr (sizeof...(args) > 1) {
//     return mr::apply(
//       get_task_prototype<ResultT, Args...>(),
//       std::forward_as_tuple<Args...>(args...)
//     );
//   }
//   else {
//     return mr::apply(
//       get_task_prototype<ResultT, Args...>(),
//       args...
//     );
//   }
// }
// 
// struct Image {
//   std::unique_ptr<uint8_t[]> pixels;
//   int height;
//   int width;
// };
// 
// template <>
// struct TaskPrototypeBuilder<Image, std::filesystem::path> {
//   inline static auto & create() {
//     static prototype = mr::Sequence {
//       [](std::filesystem::path filename) -> Image {
//         std::ifstream input_file(filename.string(), std::ios::binary | std::ios::ate);
// 
//         size_t file_size = static_cast<size_t>(input_file.tellg());
//         std::unique_ptr<uint8_t> buffer = std::make_unique<uint8_t[]>(file_size);
// 
//         input_file.seekg(0);
//         input_file.read(buffer.data(), file_size);
// 
//         int height;
//         int width;
// 
//         height = width = std::sqrt(file_size);
// 
//         assert(height * width == file_size);
// 
//         return {buffer, height, width};
//       }
//     };
//     return prototype;
//   }
// };
//
//
// int main() {
//   auto create_image_task = make_task<Image>(std::filesystem::current_path()/"image.png");
//   create_image_task->execute();
//   Image image = create_image_task->result();
// }
