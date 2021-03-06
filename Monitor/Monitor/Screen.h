#ifndef __AVS_PROXY_CLIENT_SCREEN__
#define __AVS_PROXY_CLIENT_SCREEN__

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <pjmedia-codec.h>

#include "resource.h"
#include "PoolThread.hpp"
#include "AvsProxyStructs.h"
#include "TitleRoom.h"
#include "ToolTip.h"

using std::shared_ptr;
using std::lock_guard;
using std::mutex;
using std::thread;
using std::vector;

typedef struct vid_channel
{
    unsigned		    pt;		    /**< Payload type.		    */
    pj_bool_t		    paused;	    /**< Paused?.		    */
    void		       *buf;	    /**< Output buffer.		    */
    unsigned		    buf_size;	/**< Size of output buffer.	    */
    pjmedia_rtp_session rtp;	/**< RTP session.		    */
} vid_channel_t;

typedef struct vid_stream
{
	vid_channel_t     *dec;	            /**< Decoding channel.	    */
	pj_mutex_t        *jb_mutex;
    pjmedia_jbuf      *jb;	            /**< Jitter buffer.		    */
	pjmedia_vid_codec *codec;	        /**< Codec instance being used. */
	unsigned           dec_max_size;    /**< Size of decoded/raw picture*/
	pjmedia_frame      dec_frame;	    /**< Current decoded frame.     */
	unsigned           rx_frame_cnt;    /**< # of array in rx_frames    */
    pjmedia_frame     *rx_frames;	    /**< Temp. buffer for incoming frame assembly.	    */
	pj_uint32_t		   last_dec_ts;     /**< Last decoded timestamp.    */
    int			       last_dec_seq;    /**< Last decoded sequence.     */
} vid_stream_t;

class User;
class Screen
	: public CWnd
{
public:
	Screen(pj_uint32_t index);
	virtual ~Screen();
	pj_status_t Prepare(pj_pool_t *pool, const CRect &rect, const CWnd *wrapper, pj_uint32_t);
	pj_status_t Launch();
	void        Destory();
	pj_status_t GetUser(User *&user);
	pj_status_t ConnectUser(User *user);
	pj_status_t DisconnectUser();
	inline pj_bool_t IsIdle() const { return user_ == nullptr; }
	inline pj_uint32_t GetIndex() const { return index_; }
	void MoveToRect(const CRect &);
	void HideWindow();
	void UpdateWindow();
	void Painting(const void *pixels);
	void AudioScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen);
	void OnRxAudio(const vector<pj_uint8_t> &audio_frame);
	void VideoScene(const pj_uint8_t *rtp_frame, pj_uint16_t framelen);
	void OnRxVideo(shared_ptr<pj_uint8_t> video_frame, pj_uint16_t framelen);

protected:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	pj_status_t SendTCPPacket(const void *buf, pj_ssize_t *len);
	pj_status_t decode_vid_frame();

private:
	pj_uint32_t   index_;
	mutex         render_mutex_;
	User         *user_;
	SDL_Window   *window_;
	SDL_Renderer *render_;
	SDL_Texture  *texture_;
	mutex         media_active_lock_;
	pj_bool_t     media_active_;
	pj_uint32_t   call_status_;
	vid_stream_t *stream_;
	PoolThread<std::function<void ()>> audio_thread_pool_;
	PoolThread<std::function<void ()>> video_thread_pool_;
};

#endif
