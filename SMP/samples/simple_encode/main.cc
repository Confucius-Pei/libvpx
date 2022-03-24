#include <math.h>
#include <memory>
#include <string>
#include <vector>
//#include "test/video_source.h"
#include "vp9/simple_encode.h"

using namespace vp9;
namespace {
double GetBitrateInKbps(size_t bit_size, int num_frames, int frame_rate_num,
                        int frame_rate_den) {
    return static_cast<double>(bit_size) / num_frames * frame_rate_num /
           frame_rate_den / 1000.0;
}
// Returns the number of unit in size of 4.
// For example, if size is 7, return 2.
int GetNumUnit4x4(int size) { return (size + 3) >> 2; }

const int width_ = 352;
const int height_ = 288;
const int frame_rate_num_ = 30;
const int frame_rate_den_ = 1;
const int target_bitrate_ = 1000;
const int num_frames_ = 8;
const std::string in_file_path_str_ = "./bus_352x288_420_f20_b8.yuv";
}

void SimpleEncodeTest_EncodeFrame() {
    SimpleEncode simple_encode(width_, height_, frame_rate_num_, frame_rate_den_,
                               target_bitrate_, num_frames_,
                               in_file_path_str_.c_str());
    simple_encode.ComputeFirstPassStats();
    int num_coding_frames = simple_encode.GetCodingFrameNum();
    //EXPECT_GE(num_coding_frames, num_frames_);
    simple_encode.StartEncode();
    size_t total_data_bit_size = 0;
    int coded_show_frame_count = 0;
    int frame_coding_index = 0;
    while (coded_show_frame_count < num_frames_) {
        const GroupOfPicture group_of_picture =
            simple_encode.ObserveGroupOfPicture();
        const std::vector<EncodeFrameInfo> &encode_frame_list =
            group_of_picture.encode_frame_list;
        for (size_t group_index = 0; group_index < encode_frame_list.size();
             ++group_index) {
            EncodeFrameResult encode_frame_result;
            simple_encode.EncodeFrame(&encode_frame_result);
            //EXPECT_EQ(encode_frame_result.show_idx, encode_frame_list[group_index].show_idx);
            //EXPECT_EQ(encode_frame_result.frame_type, encode_frame_list[group_index].frame_type);
            //EXPECT_EQ(encode_frame_list[group_index].coding_index, frame_coding_index);
            //EXPECT_GE(encode_frame_result.psnr, 34) << "The psnr is supposed to be greater than 34 given the "
                                                       "target_bitrate 1000 kbps";
            //EXPECT_EQ(encode_frame_result.ref_frame_info, encode_frame_list[group_index].ref_frame_info);
            total_data_bit_size += encode_frame_result.coding_data_bit_size;
            ++frame_coding_index;
        }
        coded_show_frame_count += group_of_picture.show_frame_count;
    }
    const double bitrate = GetBitrateInKbps(total_data_bit_size, num_frames_,
                                            frame_rate_num_, frame_rate_den_);
    const double off_target_threshold = 150;
    //EXPECT_LE(fabs(target_bitrate_ - bitrate), off_target_threshold);
    simple_encode.EndEncode();
}

void SimpleEncodeTest_SetExternalGroupOfPicturesMap() {
    SimpleEncode simple_encode(width_, height_, frame_rate_num_, frame_rate_den_,
                               target_bitrate_, num_frames_,
                               in_file_path_str_.c_str());
    simple_encode.ComputeFirstPassStats();

    std::vector<int> gop_map(num_frames_, 0);

    // Should be the first gop group.
    gop_map[0] = 0;

    // Second gop group with an alt ref.
    gop_map[5] |= kGopMapFlagStart | kGopMapFlagUseAltRef;

    // Third gop group without an alt ref.
    gop_map[10] |= kGopMapFlagStart;

    // Last gop group.
    gop_map[14] |= kGopMapFlagStart | kGopMapFlagUseAltRef;

    simple_encode.SetExternalGroupOfPicturesMap(gop_map.data(), gop_map.size());

    std::vector<int> observed_gop_map =
        simple_encode.ObserveExternalGroupOfPicturesMap();

    // First gop group.
    // There is always a key frame at show_idx 0 and key frame should always be
    // the start of a gop. We expect ObserveExternalGroupOfPicturesMap() will
    // insert an extra gop start here.
    //EXPECT_EQ(observed_gop_map[0], kGopMapFlagStart | kGopMapFlagUseAltRef);

    // Second gop group with an alt ref.
    //EXPECT_EQ(observed_gop_map[5], kGopMapFlagStart | kGopMapFlagUseAltRef);

    // Third gop group without an alt ref.
    //EXPECT_EQ(observed_gop_map[10], kGopMapFlagStart);

    // Last gop group. The last gop is not supposed to use an alt ref. We expect
    // ObserveExternalGroupOfPicturesMap() will remove the alt ref flag here.
    //EXPECT_EQ(observed_gop_map[14], kGopMapFlagStart);

    int ref_gop_show_frame_count_list[4] = {5, 5, 4, 3};
    size_t ref_gop_coded_frame_count_list[4] = {6, 6, 4, 3};
    int gop_count = 0;

    simple_encode.StartEncode();
    int coded_show_frame_count = 0;
    while (coded_show_frame_count < num_frames_) {
        const GroupOfPicture group_of_picture =
            simple_encode.ObserveGroupOfPicture();
        const std::vector<EncodeFrameInfo> &encode_frame_list =
            group_of_picture.encode_frame_list;
        //EXPECT_EQ(encode_frame_list.size(), ref_gop_coded_frame_count_list[gop_count]);
        //EXPECT_EQ(group_of_picture.show_frame_count, ref_gop_show_frame_count_list[gop_count]);
        for (size_t group_index = 0; group_index < encode_frame_list.size();
             ++group_index) {
            EncodeFrameResult encode_frame_result;
            simple_encode.EncodeFrame(&encode_frame_result);
        }
        coded_show_frame_count += group_of_picture.show_frame_count;
        ++gop_count;
    }
    //EXPECT_EQ(gop_count, 4);
    simple_encode.EndEncode();
}

int main(int argc, char** argv) {
    SimpleEncodeTest_EncodeFrame();
}