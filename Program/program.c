#include "stdlib.h"
#include "stdint.h"
#include "main.h"

#include "musicdata.h"
#include "common.h"

//#define MINIMAL_DELAY sleep_us(10);
//#define SLEEP_DELAY sleep_us(sleepUnit);
#define MINIMAL_DELAY HAL_Delay(1);
#define SLEEP_DELAY HAL_Delay(sleepUnit);

static void initializeChannel(channel_t *channel, TIM_TypeDef *device);
static channel_t* initializeChannels(uint8_t numChannels);
static void initializeTrack(track_t *track, channel_t *channel, const sequence_t *sequence);
static track_t* initializeTracks(uint8_t numTracks, channel_t *channels, const composition_t *pComposition);
static state_t* initializeState(const composition_t *pComposition);
static void readTrack(track_t *target, uint8_t *rhythmUnit);
static void readTracks(state_t *state);
static void playChannels(state_t *state);
static void instSilence(channel_t *channel, state_t *state);
static void instRegular(channel_t *channel, state_t *state);

static const uint64_t sleepUnit = 1;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

const instrument instruments[] =
{
    instSilence, instRegular
};

int program(void)
{
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    // initialize data
    state_t *state = initializeState(&composition);

    // take a deep breath
    HAL_Delay(500);

    // let's gooo
    for(;;)
    {
        readTracks(state);
        playChannels(state);
        SLEEP_DELAY
    }
}

static void initializeChannel(channel_t *channel, TIM_TypeDef *device)
{
    channel->timer = device;
    channel->currentTone = 0;
    for(int i = 0; i < 4; i++)
    {
        channel->currentPitches[i] = 255;
    }
    channel->currentPitchCount = 0;
    channel->nextPitchIndex = 0;
    channel->polyCycleThreshold = 24;
    channel->polyCycleCounter = 0;
    channel->instrument = instruments[0];
}

static channel_t* initializeChannels(uint8_t numChannels)
{
	static TIM_TypeDef *timers[4] = { TIM1, TIM2, TIM3, TIM4 };
    channel_t *channels = malloc((numChannels) * sizeof(channel_t));

    for (int i = 0; i < numChannels; i++)
    {
        // add 2 for first port test, use whole range later
        // TODO: remove +2
        initializeChannel(&channels[i], timers[i]);
    }
    return channels;
}

static void initializeTrack(track_t *track, channel_t *channel, const sequence_t *sequence)
{
    track->channel = channel;
    track->sequence = sequence->sequence;
    track->sLength = sequence->sequenceLength;
    track->sPosition = 0;
    track->remainingSleepTime = 0;
    track->jPosition = 0;
}

static track_t* initializeTracks(uint8_t numTracks, channel_t *channels, const composition_t *pComposition)
{
    track_t *tracks = malloc((numTracks) * sizeof(track_t));

    for (int i = 0; i < numTracks; i++)
    {
        initializeTrack(&tracks[i], &channels[i], &pComposition->sequences[i]);
    }

    return tracks;
}

static state_t* initializeState(const composition_t *pComposition)
{
    state_t *state = malloc(sizeof(state_t));
    state->rhythmUnit = pComposition->initialRhythmUnit;
    state->numChannels = pComposition->numSequences;
    state->numTracks = pComposition->numSequences;
    // initialize the channels - device usage & state management
    state->channels = initializeChannels(state->numChannels);
    // initialize the tracks - parallel streams of commands to the channels
    state->tracks = initializeTracks(state->numTracks, state->channels, pComposition);
    // giving up on ADC volume for now -
    // clearly not as straightforward as it was in the atmega,
    // and it wasn't the right way anyway
    state->volume = 16.0;

    return state;
}

static void readTrack(track_t *target, uint8_t *rhythmUnit)
{
    if (target->remainingSleepTime > 0)
    {
        target->remainingSleepTime-=sleepUnit;
        return;
    }
    
    if (target->sPosition >= target->sLength - 1)
    {
        target->remainingSleepTime = 0;
        return;
    }

    const uint32_t *tSequence = target->sequence;
    uint16_t position = target->sPosition;
    uint8_t code = tSequence[position];
    channel_t *tChannel = target->channel;

    switch (code) 
    {
        case 0:
            // sleep for duration
            target->remainingSleepTime =
                tSequence[position+1] * sleepUnit * (*rhythmUnit);
            target->sPosition = position+2;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            // Set pitches
            // pitches + sleep combo to save space
            target->remainingSleepTime =
                tSequence[position+code+1] * sleepUnit * (*rhythmUnit);
            tChannel->currentPitchCount = code;
            target->sPosition = position + code + 2;
            for (int i = 0; i < code; i++)
            {
                tChannel->currentPitches[i] = tSequence[position+i+1];
            }
            tChannel->nextPitchIndex = 0;
            break;
        case 11:
        case 12:
        case 13:
        case 14:
            code -= 10;
            // Set pitches
            // pitches + sleep + volume combo to save space
            target->channel->currentTone = tSequence[position+code+1];
            target->remainingSleepTime =
                tSequence[position+code+2] * sleepUnit * (*rhythmUnit);
            target->sPosition = position + code + 3;
            tChannel->currentPitchCount = code;
            for (int i = 0; i < code; i++)
            {
                tChannel->currentPitches[i] = tSequence[position+i+1];
            }
            tChannel->nextPitchIndex = 0;
            break;
        case 5:
            // Set "volume" (voltage)
            tChannel->currentTone =
                tSequence[position+1];
            target->sPosition = position+2;
            break;
        case 6:
            // Set instrument function
            tChannel->instrument =
                instruments[tSequence[position+1]];
            target->sPosition = position+2;
            break;
        case 7:
            // Set rhythm unit (tempo)
            (*rhythmUnit) =
                tSequence[position+1];
            target->sPosition = position+2;
            break;
        case 8:
            // Jump back (repeat)
            if (target->jPosition == position)
            {
                target->sPosition = position+2;
                target->jPosition = 0;
            }
            else
            {
                target->jPosition = position;
                target->sPosition = position-(tSequence[position+1]);
            }
            break;
        case 9:
            // volume + sleep
            tChannel->currentTone =
                tSequence[position+1];
            target->remainingSleepTime =
                tSequence[position+2] * sleepUnit * (*rhythmUnit);
            target->sPosition = position+3;
            break;
        default:
            // TODO: add some sort of warning behavior
        break;
    }
}

static void readTracks(state_t* state)
{
    track_t* tracks = state->tracks;
    // check if it's time to loop
    // and manually resync tracks
    for (int i = 0; i < state->numTracks; i++)
    {
        if (tracks[i].sPosition >= tracks[i].sLength-1
            && tracks[i].remainingSleepTime <= 0)
        {
            // if true for ONE, sync ALL then continue!
            for(int j = 0; j < state->numTracks; j++)
            {
                tracks[j].sPosition = 0;
                tracks[j].jPosition = 0;
                tracks[j].remainingSleepTime = 0;
            }
            break;
        }
    }
    
    // proceed to execute track commands
    for (int i = 0; i < state->numTracks; i++)
    {
        readTrack(&tracks[i], &state->rhythmUnit);
    }
}

static void playChannels(state_t *state)
{
    channel_t* channels = state->channels;

    for (int i = 0; i < state->numChannels; i++)
    {
        channels[i].instrument(&channels[i], state);
    }
}

static void instSilence(channel_t *channel, state_t *state)
{
}

static void instRegular(channel_t *channel, state_t *state)
{
    if (channel->currentPitchCount == 0) return;

    if (channel->nextPitchIndex >= channel->currentPitchCount)
    {
        channel->nextPitchIndex = 0;
    }

    uint32_t pitch = channel->currentPitches[channel->nextPitchIndex];
    channel->timer->ARR = pitch;
    uint32_t finalTone = (channel->currentTone)*(state->volume);
    // this specifies the timer's channel 1 - fix to all channels later
    channel->timer->CCR1 = finalTone;

    channel->polyCycleCounter++;

    if (channel->polyCycleCounter >= channel->polyCycleThreshold)
    {
        channel->polyCycleCounter = 0;
        if (channel->nextPitchIndex < channel->currentPitchCount-1)
        {
            channel->nextPitchIndex++;
        }
        else if (channel->nextPitchIndex == channel->currentPitchCount-1)
        {
            channel->nextPitchIndex = 0;
        }
    }
}
