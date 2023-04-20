#include "gtest/gtest.h"
#include <utility>
#include "google/protobuf/map.h"
#include "sample.pb.h"
#include "yaml2pb/yaml2pb.h"

const char *test_yaml = "\
name: recorder_sample\n\
metadata:\n\
  info:\n\
    my_key: my_value\n\
sources:\n\
  - name: default_source\n\
    processors:\n\
      - audio_mixer_for_wav\n\
      - audio_mixer_for_mp4\n\
      - video_mixer_for_mp4\n\
processors:\n\
  - name: video_mixer_for_mp4\n\
    type: video\n\
    modules:\n\
      - type: scaler\n\
        width: 640\n\
        height: 640\n\
      - type: h264\n\
        bitrate: 1000000\n\
        key_frame_interval: 60\n\
  - name: audio_mixer_for_mp4\n\
    type: audio\n\
    modules:\n\
      - type: resampler\n\
        sample_rate: 16000\n\
        channel_num: 1\n\
      - type: aac\n\
  - name: audio_mixer_for_wav\n\
    type: audio\n\
    modules:\n\
      - type: resampler\n\
        sample_rate: 8000\n\
        channel_num: 1\n\
      - type: pcm\n\
drains:\n\
  - name: dedicated_recording_mp4\n\
    type: mp4\n\
    processors:\n\
      - audio_mixer_for_mp4\n\
      - video_mixer_for_mp4\n\
  - name: dedicated_recording_wav\n\
    type: mp4\n\
    processors:\n\
      - audio_mixer_for_wav\n\
  - name: ondemand_recording_mp4\n\
    type: mp4\n\
    processors:\n\
      - audio_mixer_for_mp4\n\
      - video_mixer_for_mp4\n\
";

TEST(yaml2pb, sample)
{
    Sample sample;
    yaml2pb::yaml2pb(sample, test_yaml);
    EXPECT_TRUE(sample.name() == "recorder_sample");
    EXPECT_TRUE(sample.has_metadata());
    EXPECT_EQ(sample.metadata().info_size(), 1);
    EXPECT_TRUE(sample.metadata().info().at("my_key") == "my_value");
    EXPECT_EQ(sample.sources_size(), 1);
    EXPECT_TRUE(sample.sources()[0].name() == "default_source");
    EXPECT_EQ(sample.processors_size(), 3);
    EXPECT_EQ(sample.drains_size(), 3);
}

TEST(pb2yaml, sample)
{
    Sample sample;
    sample.set_name("recorder_sample");

    Source *source = sample.add_sources();
    source->set_name("default_source");
    source->add_processors("audio_mixer_for_wav");
    source->add_processors("audio_mixer_for_mp4");
    source->add_processors("video_mixer_for_mp4");

    Processor *processor = sample.add_processors();
    processor->set_name("video_mixer_for_mp4");
    processor->set_type(Processor_ProcessMediaType_video);
    Module *module = processor->add_modules();
    module->set_type(Module_ModuleType_scaler);
    module->set_width(640);
    module->set_height(640);
    module = processor->add_modules();
    module->set_type(Module_ModuleType_h264);
    module->set_bitrate(1000000);
    module->set_key_frame_interval(60);

    processor = sample.add_processors();
    processor->set_name("audio_mixer_for_mp4");
    processor->set_type(Processor_ProcessMediaType_audio);
    module = processor->add_modules();
    module->set_type(Module_ModuleType_resampler);
    module->set_sample_rate(16000);
    module->set_channel_num(1);
    module = processor->add_modules();
    module->set_type(Module_ModuleType_aac);

    processor = sample.add_processors();
    processor->set_name("audio_mixer_for_wav");
    processor->set_type(Processor_ProcessMediaType_audio);
    module = processor->add_modules();
    module->set_type(Module_ModuleType_resampler);
    module->set_sample_rate(8000);
    module->set_channel_num(1);
    module = processor->add_modules();
    module->set_type(Module_ModuleType_pcm);

    Drain *drain = sample.add_drains();
    drain->set_name("dedicated_recording_mp4");
    drain->set_type(::Drain_DrainType_mp4);
    drain->add_processors("audio_mixer_for_mp4");
    drain->add_processors("video_mixer_for_mp4");

    drain = sample.add_drains();
    drain->set_name("dedicated_recording_wav");
    drain->set_type(::Drain_DrainType_mp4);
    drain->add_processors("audio_mixer_for_wav");

    drain = sample.add_drains();
    drain->set_name("ondemand_recording_mp4");
    drain->set_type(::Drain_DrainType_mp4);
    drain->add_processors("audio_mixer_for_mp4");
    drain->add_processors("video_mixer_for_mp4");

    std::string out = yaml2pb::pb2yaml(sample);
    EXPECT_TRUE(out == test_yaml);
    std::cout << out;
}