// PinSound.cpp: implementation of the PinSound class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PinSound::PinSound()
{
	m_pDSBuffer = NULL;
	m_pdata = NULL;
}

PinSound::~PinSound()
{
	SAFE_RELEASE(m_pDSBuffer);

	if (m_pdata)
		{
		delete [] m_pdata;
		}
}

PinDirectSound::PinDirectSound()
	{
	m_pDS = NULL;
	//m_pDSBuffer = NULL;
	m_pWaveSoundRead = NULL;
	}

PinDirectSound::~PinDirectSound()
	{
	SAFE_DELETE(m_pWaveSoundRead);
	//SAFE_RELEASE(m_pDSBuffer);
    SAFE_RELEASE(m_pDS);
	}

void PinDirectSound::InitDirectSound(HWND hwnd)
{
    HRESULT hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    // Initialize COM
    //if( hr = CoInitialize( NULL ) )
        //return hr;

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &m_pDS, NULL ) ) )
		{
		ShowError("Could not create Direct Sound.");
        return;// hr;
		}

    // Set coop level to DSSCL_PRIORITY
    if( FAILED( hr = m_pDS->SetCooperativeLevel( hwnd, DSSCL_PRIORITY ) ) )
		{
		ShowError("Could not set Direct Sound Priority.");
        return;// hr;
		}

    // Get the primary buffer 
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
       
    if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
		{
		ShowError("Could not create primary sound buffer.");
        return;// hr;
		}

    // Set primary buffer format to 22kHz and 16-bit output.
    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
    wfx.wFormatTag      = WAVE_FORMAT_PCM; 
    wfx.nChannels       = 2; 
    wfx.nSamplesPerSec  = 22050; 
    wfx.wBitsPerSample  = 16; 
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) )
		{
		ShowError("Could not set sound format.");
        return;// hr;
		}

    SAFE_RELEASE( pDSBPrimary );

	//LoadWaveFile("d:\\gdk\\vbatest\\sounds\\sound18.wav");

    return;// S_OK;
}

PinSound *PinDirectSound::LoadWaveFile(TCHAR* strFileName )
{
	PinSound *pps;

	pps = new PinSound();

    // Create the sound buffer object from the wave file data
    if( FAILED( CreateStaticBuffer( strFileName, pps ) ) )
    {
		ShowError("Could not create static sound buffer.");
		delete pps;
		return NULL;
        //SetFileUI( hDlg, TEXT("Couldn't create sound buffer.") ); 
    }
    else // The sound buffer was successfully created
    {
        // Fill the buffer with wav data
        FillBuffer(pps);

        // Update the UI controls to show the sound as the file is loaded
        //SetFileUI( hDlg, strFileName );
        //OnEnablePlayUI( hDlg, TRUE );
    }

	lstrcpy(pps->m_szPath, strFileName);

	TitleFromFilename(strFileName, pps->m_szName);

	lstrcpy(pps->m_szInternalName, pps->m_szName);

	CharLowerBuff(pps->m_szInternalName, lstrlen(pps->m_szInternalName));

	return pps;
}




//-----------------------------------------------------------------------------
// Name: CreateStaticBuffer()
// Desc: Creates a wave file, sound buffer and notification events 
//-----------------------------------------------------------------------------
HRESULT PinDirectSound::CreateStaticBuffer(TCHAR* strFileName, PinSound *pps)
{
    HRESULT hr; 

    // Free any previous globals 
    SAFE_DELETE( m_pWaveSoundRead );
    //SAFE_RELEASE( m_pDSBuffer );

    // Create a new wave file class
    m_pWaveSoundRead = new CWaveSoundRead();

    // Load the wave file
    if( FAILED( m_pWaveSoundRead->Open( strFileName ) ) )
    {
		ShowError("Could not open file.");
        //SetFileUI( hDlg, TEXT("Bad wave file.") );
		return E_FAIL;
    }

    // Set up the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can 
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME;
    dsbd.dwBufferBytes = m_pWaveSoundRead->m_ckIn.cksize;
    dsbd.lpwfxFormat   = m_pWaveSoundRead->m_pwfx;

    // Create the static DirectSound buffer 
    if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pps->m_pDSBuffer, NULL ) ) )
		{
		ShowError("Could not create sound buffer.");
        return hr;
		}

    // Remember how big the buffer is
    m_dwBufferBytes = dsbd.dwBufferBytes;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FillBuffer()
// Desc: Fill the DirectSound buffer with data from the wav file
//-----------------------------------------------------------------------------
HRESULT PinDirectSound::FillBuffer(PinSound *pps)
{
    HRESULT hr; 
    BYTE*   pbWavData; // Pointer to actual wav data 
    UINT    cbWavSize; // Size of data
    VOID*   pbData  = NULL;
    VOID*   pbData2 = NULL;
    DWORD   dwLength;
    DWORD   dwLength2;

    // The size of wave data is in pWaveFileSound->m_ckIn
    INT nWaveFileSize = m_pWaveSoundRead->m_ckIn.cksize;

    // Allocate that buffer.
    pbWavData = new BYTE[ nWaveFileSize ];
    if( NULL == pbWavData )
        return E_OUTOFMEMORY;

    if( FAILED( hr = m_pWaveSoundRead->Read( nWaveFileSize, 
                                           pbWavData, 
                                           &cbWavSize ) ) )
		{
		ShowError("Could not read wav file.");         
        return hr;
		}

    // Reset the file to the beginning 
    m_pWaveSoundRead->Reset();

    // Lock the buffer down
    if( FAILED( hr = pps->m_pDSBuffer->Lock( 0, m_dwBufferBytes, &pbData, &dwLength, 
                                   &pbData2, &dwLength2, 0L ) ) )
		{
		ShowError("Could not lock sound buffer.");
        return hr;
		}

    // Copy the memory to it.
    memcpy( pbData, pbWavData, m_dwBufferBytes );

    // Unlock the buffer, we don't need it anymore.
    pps->m_pDSBuffer->Unlock( pbData, m_dwBufferBytes, NULL, 0 );
    pbData = NULL;

	pps->m_pdata = new char [m_dwBufferBytes];

	memcpy( pps->m_pdata, pbWavData, m_dwBufferBytes );

	pps->m_cdata = m_dwBufferBytes;

    // We dont need the wav file data buffer anymore, so delete it 
    SAFE_DELETE( pbWavData );

    return S_OK;
}

HRESULT PinDirectSound::CreateDirectFromNative(PinSound *pps, WAVEFORMATEX *pwfx)
	{
	DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME;
    dsbd.dwBufferBytes = pps->m_cdata;
    dsbd.lpwfxFormat   = pwfx;
	HRESULT hr;

	if (m_pDS == NULL)
		{
		pps->m_pDSBuffer = NULL;
		return E_FAIL;
		}

    // Create the static DirectSound buffer 
    if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pps->m_pDSBuffer, NULL ) ) )
		{
		ShowError("Could not create sound buffer for load.");
        return hr;
		}

    VOID*   pbData  = NULL;
    VOID*   pbData2 = NULL;
    DWORD   dwLength;
    DWORD   dwLength2;

    // Lock the buffer down
    if( FAILED( hr = pps->m_pDSBuffer->Lock( 0, pps->m_cdata, &pbData, &dwLength, 
                                   &pbData2, &dwLength2, 0L ) ) )
		{
		ShowError("Could not lock sound buffer for load.");
        return hr;
		}

    // Copy the memory to it.
    memcpy( pbData, pps->m_pdata, pps->m_cdata );

    // Unlock the buffer, we don't need it anymore.
    pps->m_pDSBuffer->Unlock( pbData, pps->m_cdata, NULL, 0 );
    pbData = NULL;

    return S_OK;
	}

PinMusic::PinMusic()
	{
	m_hmodWMA = LoadLibrary("wmaudsdk.dll");
	
	if (m_hmodWMA)
		{
		g_AudioCreateReaderFunc = (WMAudioCreateReaderFunc)GetProcAddress(m_hmodWMA,"WMAudioCreateReader");
		}
	else
		{
		g_AudioCreateReaderFunc = NULL;
		}

	//XAudPlayer xap;

	//xap.Init("D:\\Games\\AOK\\Sound\\stream\\Celt.mp3");

	//m_hmodWMA = NULL;
	//g_AudioCreateReaderFunc = NULL;
	}

void PinMusic::Foo()
	{
	/*if (g_AudioCreateReaderFunc)
		{
		m_pcsimpleplayer = new CSimplePlayer();
		HRESULT hrFoo;
		hrFoo = m_pcsimpleplayer->Play(L"d:\\songs\\raw\\wm_start.wma", NULL, &hrFoo);
		}*/
	}

PinMusic::~PinMusic()
	{
	/*if (m_pcsimpleplayer)
		{
		m_pcsimpleplayer->Stop();
		//m_csimpleplayer.m_pReader->Stop();
		//m_csimpleplayer.m_pDSBuffer->Stop();

		//m_csimpleplayer->Release();
		m_pcsimpleplayer->Release();
		}*/

	if (m_hmodWMA)
		{
		FreeLibrary(m_hmodWMA);
		}
	}
