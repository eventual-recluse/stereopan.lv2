/* include libs */
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include "lv2.h"

/* Stereo channel variables used throughout the program */
enum Channels{
    C_LEFT = 0,
    C_RIGHT = 1,
    CHANNELS = 2
};

/* LV2 port indexes. Match the port indexes defined in stereopan.ttl  */
enum Ports{
    SP_AUDIO_IN_L    = 0,
    SP_AUDIO_IN_R    = 1,
    SP_AUDIO_OUT_L   = 2,
    SP_AUDIO_OUT_R   = 3,
    SP_AMP           = 4,
    SP_WIDTH         = 5,
    SP_BALANCE       = 6
};

/* class definition */
class StereoPan 
{
private:
    /* Array of pointers for audio in and out, each set to the size of the no. of channels */
    float* audio_in_ptr[CHANNELS];
    float* audio_out_ptr[CHANNELS];
    
    /* Pointers for the control ports */
    float* amp_ptr;
    float* width_ptr;
    float* balance_ptr;
    
    /* Floats to hold variables for parameter smoothing */
    float current_amp;
    float current_width;
    float current_balance;
    
public:
    StereoPan ();
    void connectPort (const uint32_t port, void* data_location);
    void activate ();
    void run (const uint32_t sample_count);
};

StereoPan::StereoPan () :
    // Note the CURLY brackets used for assigning nullptr to the arrays
    audio_in_ptr {nullptr},
    audio_out_ptr {nullptr},
    // Brackets for the single pointers assigned nullptr
    amp_ptr (nullptr),
    width_ptr (nullptr),
    balance_ptr (nullptr),
    current_amp (1.0f),
    current_width (1.0f),
    current_balance (0.0f)
{
    
}

void StereoPan::connectPort (const uint32_t port, void* data_location)
{
    
    /* Note that audio_in_ptr and audio_out_ptr are arrays
     * and C_LEFT = 0 and C_RIGHT = 1     */
    switch (port)
    {
    case SP_AUDIO_IN_L:
        audio_in_ptr[C_LEFT] = (float*) data_location;
        break;
        
    case SP_AUDIO_IN_R:
        audio_in_ptr[C_RIGHT] = (float*) data_location;
        break;

    case SP_AUDIO_OUT_L:
        audio_out_ptr[C_LEFT] = (float*) data_location;
        break;
        
    case SP_AUDIO_OUT_R:
        audio_out_ptr[C_RIGHT] = (float*) data_location;
        break;

    case SP_AMP:
        amp_ptr = (float*) data_location;
        break;
    
    case SP_WIDTH:
        width_ptr = (float*) data_location;
        break;
        
    case SP_BALANCE:
        balance_ptr = (float*) data_location;
        break;
    
    default:
        break;
    }
}

void StereoPan::activate ()
{
    /* Set our 'current' variables (for parameter smoothing) to whatever the port values are when the plugin is activated */
    current_amp = *amp_ptr;
    current_width = *width_ptr;
    current_balance = *balance_ptr;
}

void StereoPan::run (const uint32_t sample_count)
{
    /* Check that these ports are connected. If any pointer for these ports is null (not connected), return immediately. */
    if ((!amp_ptr) || (!width_ptr) || (!balance_ptr)) return;
    /* Check our audio port pointer arrays. If any is null (not connected), return immediately. */
    for (int c = 0; c < CHANNELS; ++c)
    {
        if ((!audio_in_ptr[c]) || (!audio_out_ptr[c])) return;
    }
    /* Ensure parameter values are limited to the intended ranges */
    current_amp = current_amp < 0.0f ? 0.0f : current_amp;
    current_amp = current_amp > 2.0f ? 2.0f : current_amp;
    current_width = current_width < -1.0f ? -1.0f : current_width;
    current_width = current_width > 1.0f ? 1.0f : current_width;
    current_balance = current_balance < -1.0f ? -1.0f : current_balance;
    current_balance = current_balance > 1.0f ? 1.0f : current_balance;

    /* increments for parameter smoothing. e.g. *amp_ptr is our target amp value, to be reached by the end of processing the samples */
    float increment_amp = (*amp_ptr - current_amp) * (1.0 / (double) sample_count);
    float increment_width = (*width_ptr - current_width) * (1.0 / (double) sample_count);
    float increment_balance = (*balance_ptr - current_balance) * (1.0 / (double) sample_count);
    
    for (uint32_t i = 0; i < sample_count; ++i)
    {
        // parameter smoothing
        if (i == sample_count - 1)
        {
            /* Ensure current values equal target values at the last sample processed. */
            current_amp = *amp_ptr;
            current_width = *width_ptr;
            current_balance = *balance_ptr;
        }
        else
        {
            /* Increment values linearly. */
            current_amp += increment_amp;
            current_width += increment_width;
            current_balance += increment_balance;
        }
        
        float mid = (audio_in_ptr[C_LEFT][i] + audio_in_ptr[C_RIGHT][i]) / 2;
        float side = (audio_in_ptr[C_LEFT][i] - audio_in_ptr[C_RIGHT][i]) / 2;
	
	/* By letting width go to -1.0, we can completely reverse the L and R channels.
         * But if it's below zero we have to reverse its sign in the divisor below      */
        float sign = 1.0f;
        if (current_width < 0) { sign = - sign; }
	    
        audio_out_ptr[C_LEFT][i] = (mid + current_width * side) * 2 / (1 + (current_width * sign)) * current_amp;
        audio_out_ptr[C_RIGHT][i] = (mid - current_width * side) * 2 / (1 + (current_width * sign)) * current_amp;
        
	if (current_balance < 0)
        {
            audio_out_ptr[C_RIGHT][i] *= 1 + current_balance;
        }
        else
        {
            audio_out_ptr[C_LEFT][i] *= 1 - current_balance;	
        }
    }
}

/* internal core methods */
static LV2_Handle instantiate (const struct LV2_Descriptor *descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
{
    StereoPan* m = new StereoPan ();
    return m;
}

static void connect_port (LV2_Handle instance, uint32_t port, void *data_location)
{
    StereoPan* m = (StereoPan*) instance;
    if (m) m->connectPort (port, data_location);
}

static void activate (LV2_Handle instance)
{   
    StereoPan* m = (StereoPan*) instance;
    if (m) m->activate ();
}

static void run (LV2_Handle instance, uint32_t sample_count)
{
    StereoPan* m = (StereoPan*) instance;
    if (m) m->run (sample_count);
}

static void deactivate (LV2_Handle instance)
{
    /* not needed here */
}

static void cleanup (LV2_Handle instance)
{
    StereoPan* m = (StereoPan*) instance;
    if (m) delete m;
}

static const void* extension_data (const char *uri)
{
    return NULL;
}

/* descriptor */
static LV2_Descriptor const descriptor =
{
    "https://github.com/eventual-recluse/stereopan.lv2",
    instantiate,
    connect_port,
    activate,
    run,
    deactivate /* or NULL */,
    cleanup,
    extension_data /* or NULL */
};

/* interface */
LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor (uint32_t index)
{
    if (index == 0) return &descriptor;
    else return NULL;
}
