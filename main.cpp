#include <iostream>
#include <cstdio>
#include <Windows.h>
#include <conio.h>
#include <assert.h>
#include <mmsystem.h>
#include <time.h>
#include <sstream>
#include "portmidi.h"
#include "porttime.h"
#include "pmutil.h"
#include "pmwinmm.h"


#pragma comment(lib,"winmm.lib")

#define MAX_DATA 50000
#define STRING_MAX 256
#define INPUT_BUFFER_SIZE 100
#define OUTPUT_BUFFER_SIZE 0
#define DRIVER_INFO NULL
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define ASYNC_KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define KEY_DOWN(VK_NONAME) ((GetKeyState(VK_NONAME) & 0x8000) ? 1:0)
using namespace std;

long i;
long timestamp, channel, note, force;
int key;
int operation = KEYEVENTF_KEYUP;
int seldevice;
PmStream * midi;
PmError status; int length;
PmEvent buffer[1];
int sleeptime = 1;
int default_in;
int default_out;
int pianoclock[128];
int keystate[64];
int movement = 0;
int num_mania_key = 18;
int offset = 0;
bool 
f_checkkey_R = false, 
f_checkkey_esc = false, 
f_record_mode = false, 
f_showmidimessage = true,
f_async_click = false;
char line[STRING_MAX];

//record mode
int bpm;
int click_gap;
struct MANIA_NOTE {
	int type;
	int position;
	long timestamp1;
	long timestamp2;
};

MANIA_NOTE dataa[MAX_DATA];

int key_map[64] = {
	'A','S','D','F','G','H','J','K','L','Q','W','E','R','T','Y','U','I','O',
};
int rec_hit_map[64] = {
	14,42,71,99,128,156,184,213,241,271,298,327,355,384,412,440,469,497
};
//You need to change the mania key control settings like this in the game
//I cannot find the method to simulate the keyevent Ralt


int checkkey() {
	//Exit when ESC is pressed in the foreground
	if (kbhit() != 0) {
		if (ASYNC_KEY_DOWN(27/*ESC*/) && ASYNC_KEY_DOWN(VK_CAPITAL)) {
			return 1;
		}
		else if (ASYNC_KEY_DOWN('R') && ASYNC_KEY_DOWN(VK_CAPITAL)) {
			return 2;
		}
		else if (ASYNC_KEY_DOWN(32/*space*/)) {
			return 3;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
	
}

int get_bpm() {
	int b;
	cout << "Type the speed (bpm):";
	scanf("%d", &b);
	if (b > 0 && b < 600) {
		return b;
	}
	else {
		cout << "Invalid speed.This value should be in the range of (0,600)\n";
		return get_bpm();
	}
}

DWORD WINAPI play_click(LPVOID lpParamer) {

	int s_gap = click_gap / 2;
	if (s_gap < 1) s_gap++;
	Sleep(click_gap * 2);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	Pt_Stop();
	Pt_Start(1, 0, 0);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(click_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(s_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(s_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(s_gap);
	PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
	Sleep(s_gap);

	while (f_async_click) {
		PlaySound(TEXT("click.wav"), NULL, SND_FILENAME | SND_ASYNC);
		Sleep(click_gap);
	}
	return 0;
}



void record_mode() {
	//get speed
	bpm = get_bpm();
	if (bpm > 0 && bpm < 600) {
		click_gap = 60000 / bpm;
		cout << "Speed set!\n";
	}
	else {
		assert(0);
	}
	f_async_click = true;
	CreateThread(NULL, NULL, play_click, NULL, NULL, NULL);
	
	

	long key_data_map[64];
	i = 0;
	operation = KEYEVENTF_KEYUP;
	
	cout << "Ready to record!\n";

	
	

	while (1) {
		switch (checkkey()) {
		case 0:
			break;
		case 1:
			f_checkkey_esc = true;
			break;
		case 2:
			cout << "You are already in the record mode.\n";
			break;
		case 3:
			f_async_click = false;
			cout << "Stopped!\nSaving...";
			
			
			freopen("output.osu", "w", stdout);
			cout << "osu file format v14\n\n[General]\nAudioFilename: audio.mp3\nAudioLeadIn: 0\nPreviewTime: -1\nCountdown: 0\nSampleSet: Normal\nStackLeniency: 0.7\nMode: 3\nLetterboxInBreaks: 0\nSpecialStyle: 0\nWidescreenStoryboard: 0\n\n[Editor]\nDistanceSpacing: 2.5\nBeatDivisor: 2\nGridSize: 8\nTimelineZoom: 1\n\n[Metadata]\nTitle:\nTitleUnicode:\nArtist:\nArtistUnicode:\nCreator:\nVersion:\nSource:\nTags:\nBeatmapID:0\nBeatmapSetID:-1\n\n[Difficulty]\nHPDrainRate:5\nCircleSize:18\nOverallDifficulty:5\nApproachRate:5\nSliderMultiplier:1.4\nSliderTickRate:1\n\n[Events]\n//Background and Video events\n//Break Periods\n//Storyboard Layer 0 (Background)\n//Storyboard Layer 1 (Fail)\n//Storyboard Layer 2 (Pass)\n//Storyboard Layer 3 (Foreground)\n//Storyboard Layer 4 (Overlay)\n//Storyboard Sound Samples\n\n";
			cout << "[TimingPoints]\n";
			cout << offset <<", " << click_gap << ", 4, 1, 0, 100, 1, 0\n\n\n[HitObjects]\n";
			
			for (int j = 0; j < i; ++j) {
				if (dataa[j].type == 1) {
					cout << dataa[j].position << ",192," << dataa[j].timestamp1 << ",1,0,0:0:0:0:\n";
				}
				else {
					cout << dataa[j].position << ",192," << dataa[j].timestamp1 << ",128,0," << dataa[j].timestamp2 << ":0:0:0:0:\n";
				}
			}
			freopen("CON", "w", stdout);
			cout << "done!\n";
			f_checkkey_esc = true;
			_getch();
			break;
		default:
			assert(0);
		}

		if (i > 40000) {
			cout << "Too many notes ! Stopped!\n";
			break;
		}
		
		if (f_checkkey_esc) {
			f_checkkey_esc = false;
			break;
		}

		status = Pm_Poll(midi);
		if (status == TRUE) {
			length = Pm_Read(midi, buffer, 1);
			if (length > 0) {
				channel = (long)Pm_MessageStatus(buffer[0].message);

				//Ignore the notes which are not from channel 144 & 128
				//The notes being ignored are usually generated by pedals
				if (channel != 144 && channel != 128) {
					continue;
				}
				//Delete this first if this program doesn't work properly with your midi input device

				timestamp = (long)buffer[0].timestamp;
				note = (long)Pm_MessageData1(buffer[0].message);
				force = (long)Pm_MessageData2(buffer[0].message);
				if (force == 0) {
					operation = KEYEVENTF_KEYUP;
				}
				else {
					operation = 0;
				}
				key = (note + movement) % num_mania_key;


				if (operation == 0 && !keystate[key]) {
					keystate[key] = true;
					
					dataa[i].timestamp1 = timestamp;
					dataa[i].position = rec_hit_map[key];
					key_data_map[key] = i;
					i++;

					cout << key << "	" << operation << "\n";
				}
				if (operation != 0 && keystate[key]) {
					keystate[key] = false;
					
					if ((timestamp - dataa[key_data_map[key]].timestamp1) >= (click_gap)) {
						dataa[key_data_map[key]].timestamp2 = timestamp;
						dataa[key_data_map[key]].type = 128;
					}
					else {
						dataa[key_data_map[key]].type = 1;
					}

					cout << key << "	" << operation << "\n";
				}
				//simulate keyboard event
				//This is not the best way to map the piano with the key, but it is stable.




				//show midi message
				if (f_showmidimessage) {
					printf("note %ld: %ld, %ld %ld %ld\n",//%21x
						i, timestamp, channel, note, force);
				}



				
			}
			else {
				assert(0);
			}
		}
		Sleep(sleeptime);
	}

	
	cout << "Exit record mode!\n";
	_getch();
	return;
}

int main() {

	
	
	
	
	

	//print devices
	default_in = Pm_GetDefaultInputDeviceID();
	default_out = Pm_GetDefaultOutputDeviceID();
	for (int i = 0; i < Pm_CountDevices(); i++) {
		char *deflt;
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		
		printf("%d: %s, %s", i, info->interf, info->name);
		deflt = (i == default_in ? "default " : "");
		printf(" (%sinput)", deflt);
		printf("\n");
	}
	
	//select device
	
	cout << "Type input number: ";
	scanf("%d", &seldevice);

	//record mode access
	/*
	cout << "Press R to enter record mode.\nPress any other key to enter key-map mode\n";
	
	while (1) {
		if (kbhit()) {
			if (ASYNC_KEY_DOWN('R')) {
				//f_record_mode = true;
				cout << "Enter record mode!\n";
				cin.getline(line, STRING_MAX);
				record_mode();
				break;
			}
			else {
				//f_record_mode = false;
				cout << "Enter key-map mode!\n";
				break;
			}
		}


		Sleep(sleeptime);
	}
	*/
	i = 0;
	Pt_Start(1, 0, 0);
	Pm_OpenInput(&midi,
		seldevice,
		DRIVER_INFO,
		INPUT_BUFFER_SIZE,
		TIME_PROC,
		TIME_INFO);
	
	cout << "Midi Input opened. Reading midi messages...\nPress CapsLock + R to enter record mode.\nPress CapsLock + R to quit.\n";

	Pm_SetFilter(midi, PM_FILT_ACTIVE | PM_FILT_CLOCK | PM_FILT_SYSEX);

	while (Pm_Poll(midi)) {
		Pm_Read(midi, buffer, 1);
	}
	
	operation = KEYEVENTF_KEYUP;
	/*
	if (f_record_mode) {
		freopen("out.txt", "w", stdout);
	}
	*/
	while (1) {
		switch (checkkey()) {
			case 0:
				break;
			case 1:
				f_checkkey_esc = true;
				break;
			case 2:
				cout << "Enter record mode!\n";
				record_mode();
				i = 0;
				operation = KEYEVENTF_KEYUP;
				Sleep(1000);
				break;
			case 3:
				break;
			default:
				assert(0);
		}
			

		if (f_checkkey_esc) {
			f_checkkey_esc = false;
			break;
		}
		status = Pm_Poll(midi);
		if (status == TRUE) {
			length = Pm_Read(midi, buffer, 1);
			if (length > 0) {
				channel = (long)Pm_MessageStatus(buffer[0].message);

				//Ignore the notes which are not from channel 144 & 128
				//The notes being ignored are usually generated by pedals
				if (channel != 144 && channel != 128) {
					continue;
				}
				//Delete this first if this program doesn't work properly with your midi input device

				timestamp = (long)buffer[0].timestamp;
				note = (long)Pm_MessageData1(buffer[0].message);
				force = (long)Pm_MessageData2(buffer[0].message);
				if (force == 0) {
					operation = KEYEVENTF_KEYUP;
				}
				else {
					operation = 0;
				}
				key = (note + movement) % num_mania_key;

				
				if (operation == 0 && !keystate[key]) {
					keystate[key] = true;
					keybd_event(key_map[key], 0, operation, 0);
					cout << key << "	" << operation << "\n";
				}
				if (operation != 0 && keystate[key]) {
					keystate[key] = false;
					keybd_event(key_map[key], 0, operation, 0);
					cout << key << "	" << operation << "\n";
				}
				//simulate keyboard event
				//This is not the best way to map the piano with the key, but it is stable.
				
				
				
				
				//show midi message
				if (f_showmidimessage) {
					printf("note %ld: %ld, %ld %ld %ld\n",//%21x
							i,timestamp,channel,note,force);
				}
				
				

				i++;
			}
			else {
				assert(0);
			}
		}
		Sleep(sleeptime);
	}

	cout << "See you next time!\n";

	return 0;
}