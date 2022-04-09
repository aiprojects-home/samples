using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Tetris
{

    internal enum MusicTracks : ushort
    {
        MenuTrack = 1,
        GameTrack = 2
    }

    internal enum SFXList : ushort
    {
        SFXPushButton = 100,
        SFXLineExplode = 101,
        SFXMoveShape = 102,
        SFXRotateShape = 103,
        SFXDropShape = 104,
        SFXHitShape = 105,
        SFXPauseOn = 106,
        SFXPauseOff = 107,
    }

    internal class TAudioEngine : IDisposable
    {
        private static TAudioEngine m_Current = null;

        private bool   m_bInit = false;
        private float  m_fMusicVolume = 1.0f;
        private float  m_fSFXVolume = 1.0f;
        private bool   m_bDisposed = false;
        private bool   m_bMuteMode = false;
        private bool   m_bLoop = false;

        private MusicTracks m_nTrackId;

        public static TAudioEngine Current
        {
            get
            {
                if (m_Current == null)
                {
                    m_Current = new TAudioEngine();
                }

                return m_Current;
            }
        }

        public float MusicVolume
        {
            set
            {
                m_fMusicVolume = value;
            }
        }

        public float SFXVolume
        {
            set
            {
                m_fSFXVolume = value;
            }
        }

        public bool IsInitialized => m_bInit;

        private TAudioEngine()
        {

        }

        ~TAudioEngine()
        {
            Dispose(bDisposing: false);
            GC.SuppressFinalize(this);
        }

        public void Initialize(string strStorage, string strBankFile, int nMaxVoices, int nMaxStreams,
            int nMaxBankSize, bool bXMode)
        {
            XSEManaged.XSEInitParams InitParams;

            InitParams.strStorageFile = strStorage;
            InitParams.strBankFile = strBankFile;
            InitParams.nMaxVoices = nMaxVoices;
            InitParams.nMaxStreams = nMaxStreams;
            InitParams.nMaxBankSize = nMaxBankSize * 1024 * 1024; // MB
            InitParams.bExtendedMode = bXMode;

            try
            {
                //XSEManaged.XSoundEngine.COMInitializer(true);
                XSEManaged.XSoundEngine.Init(InitParams);
            }
            catch (Exception e)
            {
                throw e;
            }

            XSEManaged.XSoundEngine.OnPlaybackCompleteEvent += OnPlaybackComplete;
            m_bInit = true;

        }

        public void SetMuteMode(bool bMode)
        {
            if (!m_bInit)
            {
                return;
            }

            m_bMuteMode = bMode;

            if (bMode == true)
            {
                try
                {
                    XSEManaged.XSoundEngine.StopAll();
                } catch
                {
                    // LOG???
                }
            }

        }

        public void Silence()
        {
            if (m_bInit)
            {
                try
                {
                    XSEManaged.XSoundEngine.StopAll();
                } catch
                {
                    // LOG???
                }
            }
        }

        public void PlayMenuTrack()
        {
            PlayStream(MusicTracks.MenuTrack);
        }

        public void PlayGameTrack()
        {
            PlayStream(MusicTracks.GameTrack, true);
        }

        public void PlayPushButton()
        {
            PlaySFX(SFXList.SFXPushButton);
        }

        public void PlayMoveShape(float p)
        {
            PlaySFX(SFXList.SFXMoveShape, p);
        }

        public void PlayRotateShape(float p)
        {
            PlaySFX(SFXList.SFXRotateShape, p);
        }

        public void PlayDropShape(float p)
        {
            PlaySFX(SFXList.SFXDropShape, p);
        }

        public void PlayHitShape(float p)
        {
            PlaySFX(SFXList.SFXHitShape, p);
        }

        public void PlayPause(bool bOn)
        {
            if (bOn)
            {
                PlaySFX(SFXList.SFXPauseOn);
            }
            else
            {
                PlaySFX(SFXList.SFXPauseOff);
            }
        }

        public void PlayLineExplode()
        {
            PlaySFX(SFXList.SFXLineExplode);
        }

        public void Dispose()
        {
            Dispose(bDisposing: true);
        }

        private void Dispose(bool bDisposing)
        {
            if (m_bDisposed)
            {
                return;
            }

            if (bDisposing)
            {
                // Удаляем управляемые ресурсы.
            }

            if (m_bInit)
            {
                // Завершаем работу XSE.
                try
                {
                    XSEManaged.XSoundEngine.Done();
                    //XSEManaged.XSoundEngine.COMInitializer(false);
                } catch
                {
                    // LOG???
                }
            }

            m_bDisposed = true;
            m_bInit = false;
            m_Current = null;
        }

        private void OnPlaybackComplete(ushort nId)
        {
            if (!m_bMuteMode && m_bLoop)
            {
                try
                {
                    XSEManaged.XSoundEngine.PlayStream((ushort)m_nTrackId, m_fMusicVolume);
                } catch
                {
                    // LOG???
                }
            }
        }

        private void PlaySFX(SFXList nId, float fPan = 0.0f)
        {
            if (m_bInit && !m_bMuteMode)
            {
                try
                {
                    XSEManaged.XSoundEngine.Play((ushort)nId, m_fSFXVolume, fPan);
                } catch
                {
                    // LOG???
                }
            }
        }

        private void PlayStream(MusicTracks nId, bool bLoop = false)
        {
            if (m_bInit && !m_bMuteMode)
            {
                try
                {
                    XSEManaged.XSoundEngine.StopAll();
                    XSEManaged.XSoundEngine.PlayStream((ushort)nId, m_fMusicVolume);
                    m_nTrackId = nId;
                    m_bLoop = bLoop;
                }
                catch
                {
                    // LOG???
                }
            }
        }

    }
}
