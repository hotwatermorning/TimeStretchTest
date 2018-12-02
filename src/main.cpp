#include <iostream>
#include <RubberBandStretcher.h>
#include <soundtouch/SoundTouch.h>
#include <AudioFile.h>
#include <cmath>
#include <thread>
#include "./Buffer.hpp"

template<class Stretcher>
Buffer<float> stretch(Buffer<float> const &src,
                      double stretch_amount, double cent_change_amount,
                      Stretcher &st
                      );

template<>
Buffer<float> stretch(Buffer<float> const &src,
                      double stretch_amount, double cent_change_amount,
                      soundtouch::SoundTouch &st
                      )
{
    assert(stretch_amount >= 0);
    
    //　SoundTouchクラスを設定
    int const kProcessSize = 2048;
    st.setPitchSemiTones(cent_change_amount / 100.0);
    st.setTempo(1 / stretch_amount);
    
    Buffer<float> dest(src.channels(), (int)(src.samples() * stretch_amount));
    int dest_pos = 0;
    
    std::vector<float> src_interleaved(src.channels() * kProcessSize);
    std::vector<float> dest_interleaved(src.channels() * kProcessSize);
    
    auto const length = src.samples();
    
    for(int src_pos = 0; src_pos < length; src_pos += kProcessSize) {
        auto const num_send = std::min<int>(src_pos + kProcessSize, length) - src_pos;
        
        for(int ch = 0; ch < src.channels(); ++ch) {
            for(int smp = 0; smp < num_send; ++smp) {
                src_interleaved[smp * src.channels() + ch] = src.data()[ch][src_pos + smp];
            }
        }
        
        // オーディオブロックをSoundTouchクラスに送信
        st.putSamples(src_interleaved.data(), num_send/* * src.channels()*/);
        
        auto const finished = (src_pos + num_send == src.samples());
        if(finished) {
            st.flush();
        }

        for( ; ; ) {
            auto num_ready = st.numSamples()/* / src.channels() */;

            num_ready = std::min<int>(num_ready, kProcessSize);
            auto const num_receive = std::min<int>(dest_pos + num_ready, dest.samples()) - dest_pos;
            if(num_receive == 0) { break; }

            // SoundTouchクラスから取り出し可能な分だけオーディオデータを取り出して、dest_interleavedバッファに書き込み
            st.receiveSamples(dest_interleaved.data(), num_receive/* * src.channels() */);
            
            // dest_interleavedバッファからdestバッファにデータを転送
            for(int ch = 0; ch < src.channels(); ++ch) {
                for(int smp = 0; smp < num_receive; ++smp) {
                    dest.data()[ch][dest_pos + smp] = dest_interleaved[smp * src.channels() + ch];
                }
            }
            
            dest_pos += num_receive;
        }
    }
    
    return dest;
}

template<>
Buffer<float> stretch(Buffer<float> const &src,
                      double stretch_amount, double cent_change_amount,
                      RubberBand::RubberBandStretcher &st
                      )
{
    assert(stretch_amount >= 0);
    
    int const kProcessSize = 2048;
    
    //　RubberBandStretcherクラスを設定
    st.setMaxProcessSize(kProcessSize);
    st.setPitchScale(pow(2.0, cent_change_amount / 1200.0));
    st.setTimeRatio(stretch_amount);
    
    Buffer<float> dest(src.channels(), (int)(src.samples() * stretch_amount));
    int dest_pos = 0;
    
    std::vector<float const *> src_heads(src.channels());
    std::vector<float *> dest_heads(src.channels());
    
    auto const length = src.samples();
    
    for(int src_pos = 0; src_pos < length; src_pos += kProcessSize) {
        auto const num_to_send = std::min<int>(src_pos + kProcessSize, length) - src_pos;
        
        for(int ch = 0; ch < src.channels(); ++ch) {
            src_heads[ch] = src.data()[ch] + src_pos;
        }
        
        auto const finished = (src_pos + num_to_send == src.samples());
        // オーディオブロックをRubberBandStretcherクラスに送信
        st.process(src_heads.data(), num_to_send, finished);
        
        for( ; ; ) {
            auto num_ready = st.available();
            num_ready = std::min<int>(num_ready, kProcessSize);
            
            if(num_ready == -1) { return dest; } //< finished
            
            auto const num_to_receive = std::min<int>(dest_pos + num_ready, dest.samples()) - dest_pos;
            if(num_ready != 0 && num_to_receive == 0) { return dest; }
            if(num_to_receive == 0) { break; }
            
            for(int ch = 0; ch < src.channels(); ++ch) {
                dest_heads[ch] = dest.data()[ch] + dest_pos;
            }
            
            // RubberBandStretcherクラスから取り出し可能な分だけオーディオデータを取り出して、destバッファに書き込み
            st.retrieve(dest_heads.data(), num_to_receive);
            dest_pos += num_to_receive;
        }
    }
    
    assert(st.available() == -1);
    return dest;
}

enum class Library {
    kSoundTouch,
    kRubberband
};

//! @param ファイル名
//! @param stretch_amount タイムストレッチ量。
//!        2を指定すると、曲の長さが2倍になる（テンポは半分になる）。
//!        0.5を指定すると、曲の長さが半分になる（テンポは2倍になる）。
//! @param cent_change_amount ピッチシフト量。100を指定すると、100cent(i.e., 1半音)ピッチを上げる。
//! @param library タイムストレッチ／ピッチシフトに使用するライブラリを指定する。
void runner(std::string filename, double stretch_amount, double cent_change_amount, Library library)
{
    AudioFile<float> af_src;
    
    af_src.load(filename);
    
    Buffer<float> buf_src(af_src.getNumChannels(), af_src.getNumSamplesPerChannel());
    for(int ch = 0; ch < buf_src.channels(); ++ch) {
        std::copy(af_src.samples[ch].begin(),
                  af_src.samples[ch].end(),
                  buf_src.data()[ch]);
    }
    
    auto call_stretch = [&](auto &st, double stretch_ratio, double cent_change_amount) {
        return stretch(buf_src, stretch_ratio, cent_change_amount, st);
    };
    
    Buffer<float> buf_dest;
    if(library == Library::kSoundTouch) {
        soundtouch::SoundTouch st;
        st.setSampleRate(af_src.getSampleRate());
        st.setChannels(af_src.getNumChannels());
        buf_dest = call_stretch(st, stretch_amount, cent_change_amount);
    } else {
        using RB = RubberBand::RubberBandStretcher;
        
        int const options = (RB::PercussiveOptions | RB::OptionProcessRealTime);
        
        RubberBand::RubberBandStretcher st(af_src.getSampleRate(),
                                           af_src.getNumChannels(),
                                           options);
        
        buf_dest = call_stretch(st, stretch_amount, cent_change_amount);
    }
    
    
    AudioFile<float> af_dest;
    af_dest.setAudioBufferSize(af_src.getNumChannels(), buf_dest.samples());
    af_dest.setSampleRate(af_src.getSampleRate());
    af_dest.setBitDepth(16);
    
    for(auto ch = 0; ch < af_src.getNumChannels(); ++ch) {
        std::copy(buf_dest.data()[ch],
                  buf_dest.data()[ch] + buf_dest.samples(),
                  af_dest.samples[ch].begin());
    }
    
    af_dest.save(filename + "_" + (library == Library::kSoundTouch ? "soundtouch" : "rubberband") + ".wav");
}

int main()
{
    double stretch_amount = 1.3;
    double cent_change_amount = 500;
    
    runner("../../resource/test.wav", stretch_amount, cent_change_amount, Library::kSoundTouch);
    runner("../../resource/test.wav", stretch_amount, cent_change_amount, Library::kRubberband);
    
    std::cout << "finished." << std::endl;
}

