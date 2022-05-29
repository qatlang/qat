/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "./expose_box.hpp"

namespace qat {
namespace AST {

llvm::Value *ExposeBoxes::generate(IR::Generator *generator) {
  std::size_t count = 0;
  for (auto existing : generator->exposed_boxes) {
    for (auto candidate : boxes) {
      if (existing.generate() == candidate.generate()) {
        generator->throw_error(
            "Box `" + candidate.generate() +
                "` is already opened. Please remove this box" +
                (boxes.size() > 1 ? " from the list." : ""),
            file_placement);
        break;
        // NOTE - This break is currently unnecessary, but in the future, when
        // compilation is not stopped on the first encounter of an error,
        // this is needed
      } else {
        generator->exposed_boxes.push_back(candidate);
        count++;
      }
    }
  }
  for (auto sentence : sentences) {
    sentence.generate(generator);
  }
  /**
   * @brief Interating both vectors in one loop instead of two as this makes
   * more sense for this scenario
   *
   */
  auto exp_size = generator->exposed_boxes.size();
  auto boxes_size = boxes.size();
  for (auto i = exp_size - 1; ((i >= 0) && (i >= (exp_size - count))); i--) {
    if (generator->exposed_boxes.at(i).generate() ==
        boxes.at(boxes_size - (exp_size - i)).generate()) {
      generator->exposed_boxes.pop_back();
    }
  }
  return nullptr;
}

} // namespace AST
} // namespace qat