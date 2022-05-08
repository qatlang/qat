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

#ifndef QAT_UTILITIES_FILE_PLACEMENT_HPP
#define QAT_UTILITIES_FILE_PLACEMENT_HPP

#include <experimental/filesystem>

namespace fsexp = std::experimental::filesystem;

namespace qat {
namespace utilities {
/** Position indicates a line and character number in a file */
struct Position {
  /** Number of the line of this position. This will never be negative */
  unsigned long long line;

  /** Number of the character of this position. This will never be negative */
  unsigned long long character;
};

/** FilePlacement represents a particular range in a file */
class FilePlacement {
public:
  /**
   * @brief FilePlacement represents a particular range in a file
   *
   * @param file The file this placement belongs to
   * @param start The beginning position of the placement
   * @param end The ending position of the placement
   */
  FilePlacement(fsexp::path _file, Position _start, Position _end);

  /**
   * @brief FilePlacement represents a partivular range in a file
   *
   * This creates a FilePlacement from two other FilePlacements. The beginning
   * of the first one will be the beginning of the new one. The end of the
   * second one will be the end of the new one, unless the files of the provided
   * placements are different
   *
   * @param first The placement from which the start of the new placement
   * is obtained
   * @param second The placement from which the end of the new placement
   * is obtained. This argument is ignored if the files of both FilePlacements
   * don't match
   */
  FilePlacement(FilePlacement first, FilePlacement second);

  /** Path to the corresponding file */
  fsexp::path file;

  /** Starting position of the range */
  Position start;

  /** Ending position of the range */
  Position end;
};
} // namespace utilities
} // namespace qat

#endif