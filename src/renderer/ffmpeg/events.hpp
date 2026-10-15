#pragma once

#include "render_settings.hpp"

#include <Geode/loader/Event.hpp>
#include <Geode/Result.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace ffmpeg::events {
namespace impl {

    struct Dummy {};
    using writeFrame_t = geode::Result<>(Dummy::*)(std::vector<uint8_t> const&);

    class CreateRecorderEvent : public geode::ThreadSafeEvent<CreateRecorderEvent, bool(void** out)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class DeleteRecorderEvent : public geode::ThreadSafeEvent<DeleteRecorderEvent, bool(void* recorder)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class InitRecorderEvent : public geode::ThreadSafeEvent<InitRecorderEvent,
        bool(void* recorder, const RenderSettings* settings, geode::Result<>* result)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class StopRecorderEvent : public geode::ThreadSafeEvent<StopRecorderEvent, bool(void* recorder)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class GetWriteFrameFunctionEvent : public geode::ThreadSafeEvent<GetWriteFrameFunctionEvent,
        bool(writeFrame_t* out)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class CodecRecorderEvent : public geode::ThreadSafeEvent<CodecRecorderEvent,
        bool(std::vector<std::string>* out)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class MixVideoAudioEvent : public geode::ThreadSafeEvent<MixVideoAudioEvent,
        bool(const std::filesystem::path* videoFile, const std::filesystem::path* audioFile,
             const std::filesystem::path* outputMp4File, geode::Result<>* result)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

    class MixVideoRawEvent : public geode::ThreadSafeEvent<MixVideoRawEvent,
        bool(const std::filesystem::path* videoFile, const std::vector<float>* raw,
             const std::filesystem::path* outputMp4File, geode::Result<>* result)> {
    public:
        using ThreadSafeEvent::ThreadSafeEvent;
    };

}

class Recorder {
public:
    Recorder() {
        void* ptr = nullptr;
        impl::CreateRecorderEvent().send(&ptr);
        m_ptr = static_cast<impl::Dummy*>(ptr);
    }

    ~Recorder() {
        impl::DeleteRecorderEvent().send(static_cast<void*>(m_ptr));
    }

    Recorder(Recorder const&) = delete;
    Recorder& operator=(Recorder const&) = delete;

    bool isValid() const { return m_ptr != nullptr; }

    /**
     * @brief Initializes the Recorder with the specified rendering settings.
     *
     * This function configures the recorder with the given render settings,
     * allocates necessary resources, and prepares for video encoding.
     *
     * @param settings The rendering settings that define the output characteristics, 
     *                 including codec, bitrate, resolution, and pixel format.
     * 
     * @return true if initialization is successful, false otherwise.
     */
    geode::Result<> init(RenderSettings const& settings) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::InitRecorderEvent().send(static_cast<void*>(m_ptr), &settings, &result);
        return result;
    }

    /**
     * @brief Stops the recording process and finalizes the output file.
     *
     * This function ensures that all buffered frames are written to the output file,
     * releases allocated resources, and properly closes the output file.
     */
    void stop() {
        impl::StopRecorderEvent().send(static_cast<void*>(m_ptr));
    }

    /**
     * @brief Writes a single video frame to the output.
     *
     * This function takes the frame data as a byte vector and encodes it 
     * to the output file. The frame data must match the expected format and 
     * dimensions defined during initialization.
     *
     * @param frameData A vector containing the raw frame data to be written.
     * 
     * @return true if the frame is successfully written, false if there is an error.
     * 
     * @warning Ensure that the frameData size matches the expected dimensions of the frame.
     */
    geode::Result<> writeFrame(const std::vector<uint8_t>& frameData) {
        static auto writeFrame = [] {
            impl::writeFrame_t fn = nullptr;
            impl::GetWriteFrameFunctionEvent().send(&fn);
            return fn;
        }();
        if (!writeFrame) return geode::Err("Failed to call writeFrame function.");
        return std::invoke(writeFrame, m_ptr, frameData);
    }

    /**
     * @brief Retrieves a list of available codecs for video encoding.
     *
     * This function iterates through all available codecs in FFmpeg and 
     * returns a sorted vector of codec names.
     * 
     * @return A vector representing the names of available codecs.
     */
    static std::vector<std::string> getAvailableCodecs() {
        std::vector<std::string> codecs;
        impl::CodecRecorderEvent().send(&codecs);
        return codecs;
    }

private:
    impl::Dummy* m_ptr = nullptr;
};

class AudioMixer {
public:
    AudioMixer() = delete;

    /**
     * @brief Mixes a video file and an audio file into a single MP4 output.
     *
     * This function takes an input video file and an audio file, and merges them into a single MP4 output file. 
     * The output MP4 file will have both the video and audio streams synchronized.
     *
     * @param videoFile The path to the input video file.
     * @param audioFile The path to the input audio file.
     * @param outputMp4File The path where the output MP4 file will be saved.
     * 
     * @warning The audio file is expected to contain stereo (dual-channel) audio. Using other formats might lead to unexpected results.
     * @warning The video file is expected to contain a single video stream. Only the first video stream will be copied.
     */
    static geode::Result<> mixVideoAudio(std::filesystem::path const& videoFile,
                                         std::filesystem::path const& audioFile,
                                         std::filesystem::path const& outputMp4File) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::MixVideoAudioEvent().send(&videoFile, &audioFile, &outputMp4File, &result);
        return result;
    }

    /**
     * @brief Mixes a video file and raw audio data into a single MP4 output.
     *
     * This function takes an input video file and raw audio data (in the form of a vector of floating-point samples),
     * and merges them into a single MP4 output file.
     *
     * @param videoFile The path to the input video file.
     * @param raw A vector containing the raw audio data (floating-point samples).
     * @param outputMp4File The path where the output MP4 file will be saved.
     *
     * @warning The raw audio data is expected to be stereo (dual-channel). Using mono or multi-channel audio might lead to issues.
     * @warning The video file is expected to contain a single video stream. Only the first video stream will be copied.
     */
    static geode::Result<> mixVideoRaw(std::filesystem::path const& videoFile,
                                       const std::vector<float>& raw,
                                       std::filesystem::path const& outputMp4File) {
        geode::Result<> result = geode::Err("Event was not handled");
        impl::MixVideoRawEvent().send(&videoFile, &raw, &outputMp4File, &result);
        return result;
    }
};

}
