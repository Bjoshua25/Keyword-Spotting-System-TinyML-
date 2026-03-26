#ifndef _ring_buffer_h_
#define _ring_buffer_h_

#include <string.h>
#include <stdint.h>

#define SAMPLE_BUFFER_SIZE 1600

class AudioBuffer
{
public:
    int16_t samples[SAMPLE_BUFFER_SIZE];
    AudioBuffer()
    {
        memset(samples, 0, SAMPLE_BUFFER_SIZE * sizeof(int16_t));
    }
};

class RingBufferAccessor
{
private:
    AudioBuffer **m_audio_buffers;
    int m_number_audio_buffers;
    AudioBuffer *m_current_buffer;
    int m_buffer_pos;
    int m_buffer_idx;
    int m_total_size;

public:
    RingBufferAccessor(AudioBuffer **audioBuffers, int number_audio_buffers)
    {
        m_buffer_pos = 0;
        m_buffer_idx = 0;
        m_total_size = number_audio_buffers * SAMPLE_BUFFER_SIZE;
        m_audio_buffers = audioBuffers;
        m_number_audio_buffers = number_audio_buffers;
        m_current_buffer = audioBuffers[0];
    }

    // --- NEW PUBLIC METHODS FOR CLOUD UPLOADER ---
    
    // Returns the pointer to the current raw samples
    int16_t* getCurrentBuffer() {
        return m_current_buffer->samples;
    }

    // Returns the fixed size of the buffer
    int getBufferSize() {
        return SAMPLE_BUFFER_SIZE;
    }

    // --- END NEW METHODS ---

    int getIndex()
    {
        return m_buffer_idx * SAMPLE_BUFFER_SIZE + m_buffer_pos;
    }

    void setIndex(int index)
    {
        index = (index + m_total_size) % m_total_size;
        m_buffer_idx = (index / SAMPLE_BUFFER_SIZE) % m_number_audio_buffers;
        m_buffer_pos = index % SAMPLE_BUFFER_SIZE;
        m_current_buffer = m_audio_buffers[m_buffer_idx];
    }

    inline int16_t getCurrentSample()
    {
        return m_current_buffer->samples[m_buffer_pos];
    }

    inline void setCurrentSample(int16_t sample)
    {
        m_current_buffer->samples[m_buffer_pos] = sample;
    }

    inline void rewind(int samples) {
        setIndex(getIndex() - samples);
    }

    inline bool moveToNextSample()
    {
        m_buffer_pos++;
        if (m_buffer_pos == SAMPLE_BUFFER_SIZE)
        {
            m_buffer_pos = 0;
            m_buffer_idx++;
            if (m_buffer_idx == m_number_audio_buffers)
            {
                m_buffer_idx = 0;
            }
            m_current_buffer = m_audio_buffers[m_buffer_idx];
            return true;
        }
        return false;
    }
};

#endif