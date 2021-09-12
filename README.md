This is for Alienware Area 51m r2 with rx5700m only !!!!

This repo will fix the backlight adjust problem of this machine, see the code for more detail.

Still have two problem:

First, dim screen when on battery seems not work.

Second, the backlight control doesn't work when cold boot directly into Mac OS, and it will not going to work until boot into Windows/Ubuntu and then warm boot into Mac OS. Don't know the reason, may be need more initialize or reset code. And the most interesting thing is, if you boot into Ubuntu when this problem occurs, it will show some similiar errors like this.

![image](https://user-images.githubusercontent.com/46492291/132368573-15901d6a-8b5e-446b-b66d-0f7c0cf0eb18.png)

No clue how to fix this. Please help if you have any ideas!

Next trying:

Try to compare the struct pipe_ctx in AMDRadeonX6000Framebuffer.kext while not working and working for more clue

```
struct pipe_ctx {
	struct dc_plane_state *plane_state;
	struct dc_stream_state *stream;

	struct plane_resource plane_res;
	struct stream_resource stream_res;

	struct clock_source *clock_source;

	struct pll_settings pll_settings;

	uint8_t pipe_idx;

	struct pipe_ctx *top_pipe;
	struct pipe_ctx *bottom_pipe;
	struct pipe_ctx *next_odm_pipe;
	struct pipe_ctx *prev_odm_pipe;

#ifdef CONFIG_DRM_AMD_DC_DCN
	struct _vcs_dpi_display_dlg_regs_st dlg_regs;
	struct _vcs_dpi_display_ttu_regs_st ttu_regs;
	struct _vcs_dpi_display_rq_regs_st rq_regs;
	struct _vcs_dpi_display_pipe_dest_params_st pipe_dlg_param;
	int det_buffer_size_kb;
	bool unbounded_req;
#endif
	union pipe_update_flags update_flags;
	struct dwbc *dwbc;
	struct mcif_wb *mcif_wb;
	bool vtp_locked;
};
```

# Related issue

* https://bugzilla.kernel.org/show_bug.cgi?id=203905
