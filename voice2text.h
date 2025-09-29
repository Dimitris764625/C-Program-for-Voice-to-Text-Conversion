#ifndef VOICE2TEXT_H
#define VOICE2TEXT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <portaudio.h>
#include <sndfile.h>
#include <fftw3.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// Configuration constants
#define V2T_VERSION "3.0.0"
#define MAX_SENTENCE_LENGTH 4096
#define MAX_CONFIG_PATH 256
#define MAX_API_ENDPOINT 512
#define MAX_LOG_FILE 256
#define MAX_RECORDING_TIME 300
#define ENERGY_THRESHOLD 0.02f
#define ZCR_THRESHOLD 0.1f
#define MAX_WORD_LENGTH 50
#define MAX_SENTENCE_LENGTH 1000
#define MAX_WS_BUFFER_SIZE 4096

// Audio processing constants
#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 1
#define FFT_SIZE 512
#define NUM_MEL_FILTERS 40
#define NUM_MFCC_COEFFS 13
#define DEFAULT_BUFFER_DURATION_MS 10000 // 10 seconds

// Error codes
typedef enum {
    V2T_SUCCESS = 0,
    V2T_ERROR_INIT,
    V2T_ERROR_AUDIO_DEVICE,
    V2T_ERROR_MEMORY,
    V2T_ERROR_FILE_IO,
    V2T_ERROR_THREAD,
    V2T_ERROR_INVALID_PARAM,
    V2T_ERROR_NOT_IMPLEMENTED,
    V2T_ERROR_NO_DATA,
    V2T_ERROR_TIMEOUT,
    V2T_ERROR_SYSTEM,
    V2T_ERROR_NETWORK,
    V2T_ERROR_CONFIG,
    V2T_ERROR_AUDIO_PROCESSING
} v2t_error_t;


// Log levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
    LOG_LEVEL_NONE
} log_level_t;

// Configuration structure
typedef struct {
    int sample_rate;
    int channels;
    int max_recording_time;
    bool use_websocket;
    bool save_audio;
    char api_endpoint[256];
    log_level_t log_level;
        int sample_rate;
    int channels;
    char api_endpoint[MAX_API_ENDPOINT];
    char log_file[MAX_LOG_FILE];
    log_level_t log_level;
    float vad_threshold;
    int vad_silence_duration_ms;
    int vad_speech_duration_ms;
    int buffer_duration_ms;
    int api_timeout_ms;
    bool use_api;
    bool save_audio;
    bool show_version;
    bool realtime_output;
} v2t_config_t;

// Forward declarations of subsystem structures
typedef struct audio_capture_s audio_capture_t;
typedef struct feature_extractor_s feature_extractor_t;
typedef struct vad_processor_s vad_processor_t;
typedef struct api_client_s api_client_t;


// Audio buffer
typedef struct {
    float *data;
    size_t capacity;
    size_t size;
    size_t write_pos;
    size_t read_pos;
    pthread_mutex_t lock;
    float mfcc[NUM_MFCC_COEFFS];
    float energy;
    float zero_crossing_rate;
    float spectral_centroid;
    float spectral_rolloff;
    float spectral_flux;
    float pitch;
} audio_buffer_t;

// Audio features
typedef struct {
    float mfcc[NUM_MFCC_COEFFS];
    float energy;
    float zero_crossing_rate;
    float spectral_centroid;
    float spectral_rolloff;
    float spectral_flux;
} audio_features_t;



// Mel filter bank
typedef struct {
    int num_filters;
    int fft_size;
    int sample_rate;
    float **filter_bank;
    float *mel_frequencies;
    float *dct_matrix;
    float *lifter_weights;
} mel_filter_bank_t;

// Speech recognizer
typedef struct {
    mel_filter_bank_t *mel_bank;
    float noise_floor;
    int is_initialized;
    pthread_mutex_t model_lock;
} speech_recognizer_t;

// Audio recorder
typedef struct {
    audio_buffer_t buffer;
    volatile bool is_recording;
    volatile bool should_stop;
    pthread_mutex_t state_lock;
    pthread_cond_t state_cond;
} audio_recorder_t;

// Global context
typedef struct {
    audio_recorder_t recorder;
    speech_recognizer_t recognizer;
    v2t_config_t config;
    volatile sig_atomic_t signal_received;
    char output_sentence[MAX_SENTENCE_LENGTH];
    pthread_mutex_t output_lock;
    FILE *log_file;
    log_level_t log_level;
    int is_initialized;
    
    v2t_config_t config;
    audio_capture_t audio_capture;
    feature_extractor_t feature_extractor;
    vad_processor_t vad_processor;
    api_client_t api_client;
    
    volatile sig_atomic_t signal_received;
    volatile bool is_recording;
    bool is_initialized;
    
    char current_transcription[MAX_SENTENCE_LENGTH];
    pthread_mutex_t output_mutex;
} v2t_context_t;

// Public API
v2t_error_t v2t_init(void);
void v2t_cleanup(void);
v2t_error_t audio_buffer_init(audio_buffer_t *buffer, size_t capacity);
void audio_buffer_cleanup(audio_buffer_t *buffer);
v2t_error_t audio_buffer_append(audio_buffer_t *buffer, const float *data, size_t count);
v2t_error_t speech_recognizer_init(speech_recognizer_t *recognizer);
void speech_recognizer_cleanup(speech_recognizer_t *recognizer);
audio_features_t extract_features(const float *samples, size_t num_samples, mel_filter_bank_t *mel_bank);
bool is_speech(const float *samples, size_t count);
v2t_error_t save_audio_to_file(const float *samples, size_t num_samples, const char *filename);
void process_recognition_result(const char *result, size_t length);
v2t_error_t parse_arguments(int argc, char *argv[], v2t_config_t *config);
void setup_signal_handlers(void);

// Logging
#define LOG(level, fmt, ...) log_message(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
void log_message(log_level_t level, const char *file, int line, const char *fmt, ...);

// Public API functions
v2t_error_t v2t_init(void);
void v2t_cleanup(void);
void v2t_set_recording_state(bool recording);
bool v2t_get_recording_state(void);
void v2t_get_current_transcription(char *buffer, size_t size);
void v2t_update_transcription(const char *text);

// Global context access
extern v2t_context_t g_ctx;

#endif // VOICE2TEXT_H
