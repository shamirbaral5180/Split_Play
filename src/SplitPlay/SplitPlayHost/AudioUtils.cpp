#include "AudioUtils.h"
#include "Instance.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <combaseapi.h>

#include <atomic>
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace SplitPlayHost
{

// ---------------------------------------------------------------- enumeration

std::vector<AudioOutputInfo> EnumerateAudioOutputs()
{
	std::vector<AudioOutputInfo> outputs;

	// COM may already be initialised on this thread with a different model; tolerate that.
	const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	IMMDeviceEnumerator* enumerator = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
								__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
	{
		if (SUCCEEDED(comHr))
			CoUninitialize();
		return outputs;
	}

	// Work out which endpoint is the current default so we can show it first
	std::wstring defaultId;
	{
		IMMDevice* def = nullptr;
		if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &def)) && def != nullptr)
		{
			LPWSTR id = nullptr;
			if (SUCCEEDED(def->GetId(&id)) && id != nullptr)
			{
				defaultId = id;
				CoTaskMemFree(id);
			}
			def->Release();
		}
	}

	IMMDeviceCollection* collection = nullptr;
	if (SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)) && collection != nullptr)
	{
		UINT count = 0;
		collection->GetCount(&count);

		for (UINT i = 0; i < count; ++i)
		{
			IMMDevice* device = nullptr;
			if (FAILED(collection->Item(i, &device)) || device == nullptr)
				continue;

			AudioOutputInfo info{};

			LPWSTR id = nullptr;
			if (SUCCEEDED(device->GetId(&id)) && id != nullptr)
			{
				info.id = id;
				CoTaskMemFree(id);
			}

			IPropertyStore* props = nullptr;
			if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)) && props != nullptr)
			{
				PROPVARIANT value;
				PropVariantInit(&value);

				if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR)
					info.name = value.pwszVal;

				PropVariantClear(&value);

				PropVariantInit(&value);
				if (SUCCEEDED(props->GetValue(PKEY_Device_DeviceDesc, &value)) && value.vt == VT_LPWSTR)
					info.description = value.pwszVal;
				PropVariantClear(&value);

				props->Release();
			}

			if (info.name.empty())
				info.name = L"Audio output";

			info.isDefault = (!info.id.empty() && info.id == defaultId);

			outputs.push_back(std::move(info));
			device->Release();
		}

		collection->Release();
	}

	enumerator->Release();

	if (SUCCEEDED(comHr))
		CoUninitialize();

	// Default output first, then keep discovery order
	std::stable_sort(outputs.begin(), outputs.end(),
		[](const AudioOutputInfo& a, const AudioOutputInfo& b) { return a.isDefault && !b.isDefault; });

	return outputs;
}

// Shared helper: pull the friendly name and description off an endpoint
static void ReadDeviceProperties(IMMDevice* device, std::wstring& name, std::wstring& description)
{
	IPropertyStore* props = nullptr;
	if (FAILED(device->OpenPropertyStore(STGM_READ, &props)) || props == nullptr)
		return;

	PROPVARIANT value;
	PropVariantInit(&value);

	if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &value)) && value.vt == VT_LPWSTR)
		name = value.pwszVal;
	PropVariantClear(&value);

	PropVariantInit(&value);
	if (SUCCEEDED(props->GetValue(PKEY_Device_DeviceDesc, &value)) && value.vt == VT_LPWSTR)
		description = value.pwszVal;
	PropVariantClear(&value);

	props->Release();
}

// Shared helper: get the id of the current default endpoint for a flow
static std::wstring GetDefaultEndpointId(IMMDeviceEnumerator* enumerator, EDataFlow flow)
{
	std::wstring id;

	IMMDevice* def = nullptr;
	if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(flow, eConsole, &def)) && def != nullptr)
	{
		LPWSTR deviceId = nullptr;
		if (SUCCEEDED(def->GetId(&deviceId)) && deviceId != nullptr)
		{
			id = deviceId;
			CoTaskMemFree(deviceId);
		}
		def->Release();
	}

	return id;
}

std::vector<AudioInputInfo> EnumerateAudioInputs()
{
	std::vector<AudioInputInfo> inputs;

	const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	IMMDeviceEnumerator* enumerator = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
								__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
	{
		if (SUCCEEDED(comHr))
			CoUninitialize();
		return inputs;
	}

	const std::wstring defaultId = GetDefaultEndpointId(enumerator, eCapture);

	IMMDeviceCollection* collection = nullptr;
	if (SUCCEEDED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection)) && collection != nullptr)
	{
		UINT count = 0;
		collection->GetCount(&count);

		for (UINT i = 0; i < count; ++i)
		{
			IMMDevice* device = nullptr;
			if (FAILED(collection->Item(i, &device)) || device == nullptr)
				continue;

			AudioInputInfo info{};

			LPWSTR id = nullptr;
			if (SUCCEEDED(device->GetId(&id)) && id != nullptr)
			{
				info.id = id;
				CoTaskMemFree(id);
			}

			ReadDeviceProperties(device, info.name, info.description);

			if (info.name.empty())
				info.name = L"Microphone";

			info.isDefault = (!info.id.empty() && info.id == defaultId);

			inputs.push_back(std::move(info));
			device->Release();
		}

		collection->Release();
	}

	enumerator->Release();

	if (SUCCEEDED(comHr))
		CoUninitialize();

	std::stable_sort(inputs.begin(), inputs.end(),
		[](const AudioInputInfo& a, const AudioInputInfo& b) { return a.isDefault && !b.isDefault; });

	return inputs;
}

// ---------------------------------------------------------------- identify: test tone

bool PlayTestTone(const std::wstring& outputDeviceId)
{
	bool rendered = false;
	const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	IMMDeviceEnumerator* enumerator = nullptr;
	IMMDevice* device = nullptr;
	IAudioClient* client = nullptr;
	IAudioRenderClient* render = nullptr;

	do
	{
		if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
									__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
			break;

		if (FAILED(enumerator->GetDevice(outputDeviceId.c_str(), &device)) || device == nullptr)
			break;

		if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client))))
			break;

		const int sampleRate = 44100;
		const int channels = 2;
		const int bits = 16;
		const int durationMs = 450;
		const double frequency = 660.0;

		WAVEFORMATEX format{};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = channels;
		format.nSamplesPerSec = sampleRate;
		format.wBitsPerSample = bits;
		format.nBlockAlign = (WORD)(channels * bits / 8);
		format.nAvgBytesPerSec = sampleRate * format.nBlockAlign;

		if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED,
									  AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
									  10000000, 0, &format, nullptr)))
			break;

		UINT32 bufferFrames = 0;
		if (FAILED(client->GetBufferSize(&bufferFrames)) || bufferFrames == 0)
			break;

		if (FAILED(client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render))) || render == nullptr)
			break;

		const UINT32 totalFrames = (UINT32)((sampleRate * durationMs) / 1000 + bufferFrames);

		if (FAILED(client->Start()))
			break;

		UINT32 written = 0;
		double phase = 0.0;
		const double phaseStep = 2.0 * 3.14159265358979 * frequency / sampleRate;

		while (written < totalFrames)
		{
			UINT32 padding = 0;
			if (FAILED(client->GetCurrentPadding(&padding)))
				break;

			UINT32 free = bufferFrames - padding;
			if (free == 0)
			{
				Sleep(5);
				continue;
			}

			UINT32 toWrite = totalFrames - written;
			if (toWrite > free)
				toWrite = free;

			BYTE* out = nullptr;
			if (FAILED(render->GetBuffer(toWrite, &out)) || out == nullptr)
				break;

			short* samples = reinterpret_cast<short*>(out);

			for (UINT32 i = 0; i < toWrite; ++i)
			{
				// Smooth envelope so the tone does not click at the edges
				const UINT32 globalIndex = written + i;
				const double progress = (double)globalIndex / (double)totalFrames;
				const double envelope = progress < 0.05 ? progress / 0.05
									  : (progress > 0.85 ? (1.0 - progress) / 0.15 : 1.0);

				const short value = (short)(0.28 * envelope * 32767.0 * sin(phase));
				phase += phaseStep;
				if (phase > 2.0 * 3.14159265358979)
					phase -= 2.0 * 3.14159265358979;

				samples[i * 2 + 0] = value;
				samples[i * 2 + 1] = value;
			}

			render->ReleaseBuffer(toWrite, 0);
			written += toWrite;
		}

		Sleep(durationMs);
		client->Stop();

		rendered = (written > 0);

	} while (false);

	if (render != nullptr) render->Release();
	if (client != nullptr) client->Release();
	if (device != nullptr) device->Release();
	if (enumerator != nullptr) enumerator->Release();

	if (SUCCEEDED(comHr))
		CoUninitialize();

	return rendered;
}

// ---------------------------------------------------------------- mic meter

namespace
{
	struct MicWatch
	{
		std::wstring id;
		std::atomic<float> level{ 0.0f };
		std::atomic<bool> stop{ false };
		std::thread thread;
	};

	std::mutex g_micMutex;
	std::vector<std::unique_ptr<MicWatch>> g_micWatches;

	void MicWatchLoop(MicWatch* watch)
	{
		const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		IMMDeviceEnumerator* enumerator = nullptr;
		IMMDevice* device = nullptr;
		IAudioClient* client = nullptr;
		IAudioCaptureClient* capture = nullptr;
		IAudioMeterInformation* meter = nullptr;

		do
		{
			if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
										__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
				break;

			if (FAILED(enumerator->GetDevice(watch->id.c_str(), &device)) || device == nullptr)
				break;

			if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client))))
				break;

			WAVEFORMATEX* mixFormat = nullptr;
			if (FAILED(client->GetMixFormat(&mixFormat)) || mixFormat == nullptr)
				break;

			const HRESULT initHr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, mixFormat, nullptr);
			CoTaskMemFree(mixFormat);

			if (FAILED(initHr))
				break;

			if (FAILED(client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture))) || capture == nullptr)
				break;

			// The meter gives a cheap 0..1 peak without us having to read the audio
			if (FAILED(client->GetService(__uuidof(IAudioMeterInformation), reinterpret_cast<void**>(&meter))) || meter == nullptr)
				break;

			if (FAILED(client->Start()))
				break;

			while (!watch->stop)
			{
				float peak = 0.0f;
				if (SUCCEEDED(meter->GetPeakValue(&peak)))
					watch->level.store(peak);

				Sleep(50);
			}

			client->Stop();

		} while (false);

		if (meter != nullptr) meter->Release();
		if (capture != nullptr) capture->Release();
		if (client != nullptr) client->Release();
		if (device != nullptr) device->Release();
		if (enumerator != nullptr) enumerator->Release();

		if (SUCCEEDED(comHr))
			CoUninitialize();
	}
}

void WatchMicrophone(const std::wstring& inputDeviceId)
{
	if (inputDeviceId.empty())
		return;

	std::lock_guard<std::mutex> lock(g_micMutex);

	for (const auto& watch : g_micWatches)
	{
		if (watch->id == inputDeviceId)
			return; // already watching
	}

	auto watch = std::make_unique<MicWatch>();
	watch->id = inputDeviceId;
	watch->thread = std::thread(MicWatchLoop, watch.get());

	g_micWatches.push_back(std::move(watch));
}

float GetMicrophoneLevel(const std::wstring& inputDeviceId)
{
	std::lock_guard<std::mutex> lock(g_micMutex);

	for (const auto& watch : g_micWatches)
	{
		if (watch->id == inputDeviceId)
			return watch->level.load();
	}

	return 0.0f;
}

void PruneMicrophoneWatches(const std::vector<std::wstring>& keepIds)
{
	std::vector<std::unique_ptr<MicWatch>> removed;

	{
		std::lock_guard<std::mutex> lock(g_micMutex);

		for (auto it = g_micWatches.begin(); it != g_micWatches.end();)
		{
			const bool keep = std::find(keepIds.begin(), keepIds.end(), (*it)->id) != keepIds.end();

			if (keep)
			{
				++it;
			}
			else
			{
				(*it)->stop = true;
				removed.push_back(std::move(*it));
				it = g_micWatches.erase(it);
			}
		}
	}

	// Join outside the lock so a worker can finish without deadlocking
	for (auto& watch : removed)
	{
		if (watch->thread.joinable())
			watch->thread.join();
	}
}

// ---------------------------------------------------------------- routing
//
// Process loopback (Windows 10 2004+) lets us capture ONLY the target process's
// audio. We then render that audio to each chosen output device using WASAPI
// shared mode with automatic format conversion. This is route-only: we do not
// block other apps from the physical device, we just send this app's sound there.

namespace
{

// The capture format we ask process loopback for. 48kHz stereo float is widely
// supported and is converted per-output by WASAPI (AUTOCONVERTPCM).
constexpr int kSampleRate = 48000;
constexpr int kChannels = 2;
constexpr int kBitsPerSample = 32; // 32-bit float

// Handles process-loopback activation and signals the worker when ready.
class ActivationHandler : public IActivateAudioInterfaceCompletionHandler
{
public:
	explicit ActivationHandler(HANDLE readyEvent) : m_readyEvent(readyEvent) {}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
	{
		if (ppv == nullptr)
			return E_POINTER;

		if (riid == __uuidof(IActivateAudioInterfaceCompletionHandler) ||
			riid == __uuidof(IUnknown))
		{
			*ppv = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
	ULONG STDMETHODCALLTYPE Release() override
	{
		const ULONG ref = --m_ref;
		if (ref == 0)
			delete this;
		return ref;
	}

	HRESULT STDMETHODCALLTYPE ActivateCompleted(IActivateAudioInterfaceAsyncOperation* operation) override
	{
		HRESULT hrActivate = E_FAIL;
		IUnknown* unknown = nullptr;

		if (operation != nullptr)
			operation->GetActivateResult(&hrActivate, &unknown);

		if (SUCCEEDED(hrActivate) && unknown != nullptr)
			unknown->QueryInterface(__uuidof(IAudioClient), reinterpret_cast<void**>(&m_audioClient));

		if (unknown != nullptr)
			unknown->Release();

		m_result = hrActivate;
		SetEvent(m_readyEvent);
		return S_OK;
	}

	HRESULT Result() const { return m_result; }
	IAudioClient* TakeAudioClient()
	{
		IAudioClient* client = m_audioClient;
		m_audioClient = nullptr;
		return client;
	}

private:
	std::atomic<ULONG> m_ref{ 1 };
	HANDLE m_readyEvent = nullptr;
	HRESULT m_result = E_FAIL;
	IAudioClient* m_audioClient = nullptr;
};

struct RenderTarget
{
	IAudioClient* client = nullptr;
	IAudioRenderClient* render = nullptr;
	UINT32 bufferFrames = 0;
};

class AudioRoute
{
public:
	AudioRoute(unsigned long pid, std::vector<std::wstring> deviceIds)
		: m_pid(pid), m_deviceIds(std::move(deviceIds)) {}

	~AudioRoute() { Stop(); }

	bool Start()
	{
		m_thread = std::thread([this] { ThreadMain(); });
		return true;
	}

	void Stop()
	{
		m_stop = true;

		if (m_thread.joinable())
			m_thread.join();
	}

	bool Running() const { return m_running && !m_stop; }
	bool MatchesPid(unsigned long pid) const { return m_pid == pid; }

private:
	// Opens one output device (render) with the shared capture format.
	RenderTarget* OpenOutput(const std::wstring& deviceId)
	{
		IMMDeviceEnumerator* enumerator = nullptr;
		IMMDevice* device = nullptr;
		IAudioClient* client = nullptr;
		IAudioRenderClient* render = nullptr;

		if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
									__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator))))
			return nullptr;

		HRESULT hr = enumerator->GetDevice(deviceId.c_str(), &device);
		enumerator->Release();

		if (FAILED(hr) || device == nullptr)
			return nullptr;

		hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client));
		device->Release();

		if (FAILED(hr) || client == nullptr)
			return nullptr;

		WAVEFORMATEX* mixFormat = nullptr;
		if (FAILED(client->GetMixFormat(&mixFormat)) || mixFormat == nullptr)
		{
			client->Release();
			return nullptr;
		}

		// Ask WASAPI to convert our shared loopback format to this device's format
		WAVEFORMATEX request{};
		request.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		request.nChannels = kChannels;
		request.nSamplesPerSec = kSampleRate;
		request.wBitsPerSample = kBitsPerSample;
		request.nBlockAlign = (WORD)(request.nChannels * request.wBitsPerSample / 8);
		request.nAvgBytesPerSec = request.nSamplesPerSec * request.nBlockAlign;
		request.cbSize = 0;

		hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
							   AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
							   10000000, 0, &request, nullptr);

		CoTaskMemFree(mixFormat);

		if (FAILED(hr))
		{
			client->Release();
			return nullptr;
		}

		UINT32 bufferFrames = 0;
		if (FAILED(client->GetBufferSize(&bufferFrames)) || bufferFrames == 0)
		{
			client->Release();
			return nullptr;
		}

		if (FAILED(client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&render))) || render == nullptr)
		{
			client->Release();
			return nullptr;
		}

		auto* target = new RenderTarget{};
		target->client = client;
		target->render = render;
		target->bufferFrames = bufferFrames;

		client->Start();

		return target;
	}

	static void CloseOutput(RenderTarget* target)
	{
		if (target == nullptr)
			return;

		if (target->client != nullptr)
		{
			target->client->Stop();
			target->client->Release();
		}

		if (target->render != nullptr)
			target->render->Release();

		delete target;
	}

	void ThreadMain()
	{
		// Each worker owns its own COM apartment
		const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		if (OpenCaptureAndRun())
			m_running = true;

		// Cleanup
		for (auto* target : m_outputs)
			CloseOutput(target);

		m_outputs.clear();

		if (m_captureClient != nullptr)
		{
			m_captureClient->Release();
			m_captureClient = nullptr;
		}

		if (m_capture != nullptr)
		{
			m_capture->Stop();
			m_capture->Release();
			m_capture = nullptr;
		}

		m_running = false;

		if (SUCCEEDED(comHr))
			CoUninitialize();
	}

	bool OpenCaptureAndRun()
	{
		HANDLE readyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (readyEvent == nullptr)
			return false;

		AUDIOCLIENT_ACTIVATION_PARAMS params{};
		params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
		params.ProcessLoopbackParams.TargetProcessId = (DWORD)m_pid;
		params.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

		PROPVARIANT activateParams;
		PropVariantInit(&activateParams);
		activateParams.vt = VT_BLOB;
		activateParams.blob.cbSize = sizeof(params);
		activateParams.blob.pBlobData = reinterpret_cast<BYTE*>(&params);

		auto* handler = new ActivationHandler(readyEvent);

		IActivateAudioInterfaceAsyncOperation* asyncOp = nullptr;

		HRESULT hr = ActivateAudioInterfaceAsync(
			VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
			__uuidof(IAudioClient),
			&activateParams,
			handler,
			&asyncOp);

		if (FAILED(hr))
		{
			handler->Release();
			CloseHandle(readyEvent);
			return false;
		}

		WaitForSingleObject(readyEvent, 10000);
		CloseHandle(readyEvent);

		if (asyncOp != nullptr)
			asyncOp->Release();

		IAudioClient* capture = handler->TakeAudioClient();
		const HRESULT activateResult = handler->Result();
		handler->Release();

		if (FAILED(activateResult) || capture == nullptr)
			return false;

		m_capture = capture;

		WAVEFORMATEX format{};
		format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format.nChannels = kChannels;
		format.nSamplesPerSec = kSampleRate;
		format.wBitsPerSample = kBitsPerSample;
		format.nBlockAlign = (WORD)(format.nChannels * format.wBitsPerSample / 8);
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		// Loopback capture, polled (no event callback) for simplicity
		hr = m_capture->Initialize(AUDCLNT_SHAREMODE_SHARED,
								   AUDCLNT_STREAMFLAGS_LOOPBACK,
								   10000000, 0, &format, nullptr);

		if (FAILED(hr))
			return false;

		if (FAILED(m_capture->GetBufferSize(&m_captureFrames)) || m_captureFrames == 0)
			return false;

		if (FAILED(m_capture->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&m_captureClient))) ||
			m_captureClient == nullptr)
			return false;

		// Open every requested output
		for (const auto& id : m_deviceIds)
		{
			if (RenderTarget* target = OpenOutput(id))
				m_outputs.push_back(target);
		}

		if (m_outputs.empty())
			return false;

		hr = m_capture->Start();
		if (FAILED(hr))
			return false;

		// Main pump
		std::vector<BYTE> pending;

		while (!m_stop)
		{
			BYTE* data = nullptr;
			UINT32 frames = 0;
			DWORD flags = 0;

			// Drain everything the capture has for us
			for (;;)
			{
				UINT32 packet = 0;
				if (FAILED(m_captureClient->GetNextPacketSize(&packet)) || packet == 0)
					break;

				if (FAILED(m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
					break;

				const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

				if (!silent && data != nullptr && frames > 0)
				{
					const size_t bytes = (size_t)frames * kChannels * (kBitsPerSample / 8);
					pending.insert(pending.end(), data, data + bytes);
				}
				else if (frames > 0)
				{
					// Silent packet: append matching silence so timing stays aligned
					const size_t bytes = (size_t)frames * kChannels * (kBitsPerSample / 8);
					pending.insert(pending.end(), bytes, 0);
				}

				m_captureClient->ReleaseBuffer(frames);
			}

			const size_t frameBytes = (size_t)kChannels * (kBitsPerSample / 8);
			const size_t availableFrames = frameBytes > 0 ? pending.size() / frameBytes : 0;

			if (availableFrames == 0)
			{
				Sleep(5);
				continue;
			}

			// Feed every output. We push the same audio to each; anything that does not
			// fit is dropped this round (best-effort route-only forwarding).
			for (auto* target : m_outputs)
			{
				UINT32 padding = 0;
				if (FAILED(target->client->GetCurrentPadding(&padding)))
					continue;

				const UINT32 free = target->bufferFrames - padding;
				if (free == 0)
					continue;

				const UINT32 toWrite = (UINT32)(availableFrames < free ? availableFrames : free);

				BYTE* out = nullptr;
				if (SUCCEEDED(target->render->GetBuffer(toWrite, &out)) && out != nullptr)
				{
					memcpy(out, pending.data(), (size_t)toWrite * frameBytes);
					target->render->ReleaseBuffer(toWrite, 0);
				}
			}

			pending.clear();
		}

		if (m_capture != nullptr)
			m_capture->Stop();

		return true;
	}

	unsigned long m_pid = 0;
	std::vector<std::wstring> m_deviceIds;

	std::thread m_thread;
	std::atomic<bool> m_stop{ false };
	std::atomic<bool> m_running{ false };

	IAudioClient* m_capture = nullptr;
	IAudioCaptureClient* m_captureClient = nullptr;
	UINT32 m_captureFrames = 0;
	std::vector<RenderTarget*> m_outputs;
};

std::mutex g_routesMutex;
std::vector<std::unique_ptr<AudioRoute>> g_routes;

} // namespace

bool StartAudioRouting(unsigned long pid, const std::vector<std::wstring>& outputDeviceIds)
{
	if (pid == 0 || outputDeviceIds.empty())
		return false;

	// Only one routing session per process
	StopAudioRouting(pid);

	auto route = std::make_unique<AudioRoute>(pid, outputDeviceIds);

	if (!route->Start())
		return false;

	std::lock_guard<std::mutex> lock(g_routesMutex);
	g_routes.push_back(std::move(route));

	return true;
}

void StopAudioRouting(unsigned long pid)
{
	std::lock_guard<std::mutex> lock(g_routesMutex);

	for (auto it = g_routes.begin(); it != g_routes.end();)
	{
		if ((*it)->MatchesPid(pid))
		{
			(*it)->Stop();
			it = g_routes.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void StopAllAudioRouting()
{
	std::lock_guard<std::mutex> lock(g_routesMutex);

	for (auto& route : g_routes)
		route->Stop();

	g_routes.clear();
}

}
