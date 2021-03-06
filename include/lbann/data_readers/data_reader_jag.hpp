////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2016, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the LBANN Research Team (B. Van Essen, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-697807.
// All rights reserved.
//
// This file is part of LBANN: Livermore Big Artificial Neural Network
// Toolkit. For details, see http://software.llnl.gov/LBANN or
// https://github.com/LLNL/LBANN.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////

#ifndef _DATA_READER_JAG_HPP_
#define _DATA_READER_JAG_HPP_

#include "cnpy.h"
#include <string>
#include <vector>
#include "lbann/base.hpp"
#include "lbann/data_readers/opencv.hpp"
#include "data_reader.hpp"

namespace lbann {

/**
 * Loads the pairs of JAG simulation inputs and results
 */
class data_reader_jag : public generic_data_reader {
 public:
  using data_t = double;
  using scalar_t = double;
  using input_t = double;
  /**
   * Mode of modeling.
   * - Inverse: image to input param
   * - AutoI: image to image
   * - AutoS: scalar to scalar  
   */
  enum model_mode_t {Inverse, AutoI, AutoS};

  data_reader_jag(bool shuffle = true);
  // TODO: copy constructor and assignment operator for deep-copying if needed
  // The cnpy structure relies on shared_ptr
  data_reader_jag(const data_reader_jag&) = default;
  data_reader_jag& operator=(const data_reader_jag&) = default;
  ~data_reader_jag() override;
  data_reader_jag* copy() const override { return new data_reader_jag(*this); }

  std::string get_type() const override {
    return "data_reader_jag";
  }

  /// Set the modeling mode: Inverse, AutoI, or AutoS
  void set_model_mode(const model_mode_t mm);

  /// Set normalization mode: 0 = none, 1 = dataset-wise, 2 = image-wise
  void set_normalization_mode(int mode);

  /// Set the image dimension
  void set_image_dims(const int width, const int height);

  /// Load data and do data reader's chores.
  void load() override;

  /// Show the description
  std::string get_description() const;

  /// Return the number of samples
  size_t get_num_samples() const;

  /// Return the linearized size of an image
  size_t get_linearized_image_size() const;
  /// Return the linearized size of scalar outputs
  size_t get_linearized_scalar_size() const;
  /// Return the linearized size of inputs
  size_t get_linearized_input_size() const;

  int get_linearized_data_size() const override;
  int get_linearized_response_size() const override;
  const std::vector<int> get_data_dims() const override;

  /// Return the pointer to the raw image data
  data_t* get_image_ptr(const size_t i) const;
  /// Return the image data as a 1-D vector of lbann::DataType
  cv::Mat get_image(const size_t i) const;

  /// Return the pointer to the raw scalar data
  scalar_t* get_scalar_ptr(const size_t i) const;
  /// Return the scalar values of the i-th sample
  std::vector<DataType> get_scalar(const size_t i) const;

  /// Return the pointer to the raw input data
  input_t* get_input_ptr(const size_t i) const;
  /// Return the input values of the simulation correspoding to the i-th sample
  std::vector<DataType> get_input(const size_t i) const;

  void save_image(Mat& pixels, const std::string filename, bool do_scale = true) override;

 protected:
  bool fetch_datum(Mat& X, int data_id, int mb_idx, int tid) override;
  bool fetch_response(Mat& Y, int data_id, int mb_idx, int tid) override;

  /**
   * Load the data in the numpy format file.
   * Use only first_n available samples if specified.
   */
  void load(const std::string image_file, const std::string scalar_file,
            const std::string input_file, const size_t first_n = 0u);

  /// Check the dimensions of loaded data
  bool check_data(size_t& num_samples) const;

  /**
   * Normalize image data to [0 1] scale once after loading based on the mode
   *  0 (none): no normalization
   *  1 (dataset-wise): map the min/max of all the pixels in image data to 0/1
   *  2 (image-wise): map the min/max of all the pixels in current image to 0/1
   */
  void normalize_image();

  /// Set the linearized size of an image
  void set_linearized_image_size();
  /// Set the linearized size of scalar outputs
  void set_linearized_scalar_size();
  /// Return the linearized size of inputs
  void set_linearized_input_size();

  /// Return the maximum element of all the images
  data_t get_image_max() const;
  /// Return the minimum element of all the images
  data_t get_image_min() const;

 protected:
  /// The current mode of modeling
  model_mode_t m_model_mode;
  /// Whether image output data have been loaded
  bool m_image_loaded; 
  /// Whether scalar output data have been loaded
  bool m_scalar_loaded; 
  /// Whether simulation input data have been loaded
  bool m_input_loaded; 

  /// The number of samples
  size_t m_num_samples;
  /// The linearized size of an image
  size_t m_linearized_image_size;
  /// The linearized size of scalar outputs
  size_t m_linearized_scalar_size;
  /// The linearized size of inputs
  size_t m_linearized_input_size;

  /// image normalization mode
  int m_image_normalization;
  int m_image_width; ///< image width
  int m_image_height; ///< image height

  /// List of jag output images
  cnpy::NpyArray m_images;
  /// List of jag scalar outputs
  cnpy::NpyArray m_scalars;
  /// List of jag input 
  cnpy::NpyArray m_inputs;

  /// The smallest pixel value in image data (useful for normalization or visualization)
  data_t m_img_min;
  /// The largest pixel value in image data (useful for normalization or visualization)
  data_t m_img_max;
};

template<typename T>
inline void set_minibatch_item(Mat& M, const int mb_idx, const T* const ptr, const size_t count) {
  if ((count > 0u) && (ptr == nullptr)) {
    throw lbann_exception(std::string{} + __FILE__ + " " + std::to_string(__LINE__) +
                          " :: attempt to dereference a nullptr ");
  }
  for (size_t i = 0u; i < count; ++i) {
    M.Set(static_cast<El::Int>(i), static_cast<El::Int>(mb_idx), static_cast<DataType>(ptr[i]));
  }
}

} // end of namespace lbann
#endif // _DATA_READER_JAG_HPP_
