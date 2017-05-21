
/*
 * Copyright 2002, SuSE Linux AG, Author: Egbert Eich
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

/* identifying atom needed by magnifiers */
#include <X11/Xatom.h>
#include "property.h"

#include "xf86cmap.h"

#include "xf86fbman.h"

#include "fb.h"

#include "picturestr.h"

#ifdef XvExtension
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#endif

/*
 * Driver data structures.
 */
#include "dummy.h"

/* These need to be checked */
#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "servermd.h"
#ifdef USE_DGA
#define _XF86DGA_SERVER_
#include <X11/extensions/xf86dgaproto.h>
#endif

#ifdef SPARKLE_MODE
#include "xf86Crtc.h"
#endif

/* Mandatory functions */
static const OptionInfoRec *	DUMMYAvailableOptions(int chipid, int busid);
static void     DUMMYIdentify(int flags);
static Bool     DUMMYProbe(DriverPtr drv, int flags);
static Bool     DUMMYPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     DUMMYScreenInit(SCREEN_INIT_ARGS_DECL);
static Bool     DUMMYEnterVT(VT_FUNC_ARGS_DECL);
static void     DUMMYLeaveVT(VT_FUNC_ARGS_DECL);
static Bool     DUMMYCloseScreen(CLOSE_SCREEN_ARGS_DECL);
#ifndef SPARKLE_MODE
static Bool     DUMMYCreateWindow(WindowPtr pWin);
#endif
static void     DUMMYFreeScreen(FREE_SCREEN_ARGS_DECL);
static ModeStatus DUMMYValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode,
                                 Bool verbose, int flags);
static Bool	DUMMYSaveScreen(ScreenPtr pScreen, int mode);

/* Internally used functions */
static Bool     dummyModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	dummySave(ScrnInfoPtr pScrn);
static void	dummyRestore(ScrnInfoPtr pScrn, Bool restoreText);
static Bool	dummyDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
				pointer ptr);


/* static void     DUMMYDisplayPowerManagementSet(ScrnInfoPtr pScrn, */
/* 				int PowerManagementMode, int flags); */

#ifdef SPARKLE_MODE

static Bool DUMMYUpdateModes(ScrnInfoPtr pScrn, int width, int height);

static Bool DUMMYOpenSharedResources(ScreenPtr pScreen);
static Bool DUMMYCloseSharedResources(ScreenPtr pScreen);

static Bool DUMMYCreateScreenResources(ScreenPtr pScreen);
static void DUMMYBlockHandler(BLOCKHANDLER_ARGS_DECL);

static Bool DUMMYCrtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation, int x, int y);

#endif

#define DUMMY_VERSION 4000

#ifndef SPARKLE_MODE
#define DUMMY_NAME "DUMMY"
#define DUMMY_DRIVER_NAME "dummy"
#else
#define DUMMY_NAME "SPARKLEVIDEO"
#define DUMMY_DRIVER_NAME "sparklevideo"
#endif

#define DUMMY_MAJOR_VERSION PACKAGE_VERSION_MAJOR
#define DUMMY_MINOR_VERSION PACKAGE_VERSION_MINOR
#define DUMMY_PATCHLEVEL PACKAGE_VERSION_PATCHLEVEL

#define DUMMY_MAX_WIDTH 32767
#define DUMMY_MAX_HEIGHT 32767

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;


/*
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

#ifndef SPARKLE_MODE
_X_EXPORT DriverRec DUMMY = {
#else
_X_EXPORT DriverRec SPARKLEVIDEO = {
#endif
    DUMMY_VERSION,
    DUMMY_DRIVER_NAME,
    DUMMYIdentify,
    DUMMYProbe,
    DUMMYAvailableOptions,
    NULL,
    0,
    dummyDriverFunc
};

static SymTabRec DUMMYChipsets[] = {
#ifndef SPARKLE_MODE
    { DUMMY_CHIP,   "dummy" },
#else
    { DUMMY_CHIP,   "sparklevideo" },
#endif
    { -1,		 NULL }
};

typedef enum {
    OPTION_SW_CURSOR
} DUMMYOpts;

static const OptionInfoRec DUMMYOptions[] = {
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { -1,                  NULL,           OPTV_NONE,	{0}, FALSE }
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(dummySetup);

static XF86ModuleVersionInfo dummyVersRec =
{
#ifndef SPARKLE_MODE
	"dummy",
#else
    "sparklevideo",
#endif
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	DUMMY_MAJOR_VERSION, DUMMY_MINOR_VERSION, DUMMY_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData
 */
#ifndef SPARKLE_MODE
_X_EXPORT XF86ModuleData dummyModuleData = { &dummyVersRec, dummySetup, NULL };
#else
_X_EXPORT XF86ModuleData sparklevideoModuleData = { &dummyVersRec, dummySetup, NULL };
#endif

static pointer
dummySetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
#ifndef SPARKLE_MODE
        xf86AddDriver(&DUMMY, module, HaveDriverFuncs);
#else
        xf86AddDriver(&SPARKLEVIDEO, module, HaveDriverFuncs);
#endif

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * The return value must be non-NULL on success even though there
	 * is no TearDownProc.
	 */
	return (pointer)1;
    } else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

#endif /* XFree86LOADER */

static Bool
DUMMYGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate a DUMMYRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(DUMMYRec), 1);

    if (pScrn->driverPrivate == NULL)
	return FALSE;
        return TRUE;
}

static void
DUMMYFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static const OptionInfoRec *
DUMMYAvailableOptions(int chipid, int busid)
{
    return DUMMYOptions;
}

/* Mandatory */
static void
DUMMYIdentify(int flags)
{
#ifndef SPARKLE_MODE
    xf86PrintChipsets(DUMMY_NAME, "Driver for Dummy chipsets",
			DUMMYChipsets);
#endif
}

/* Mandatory */
static Bool
DUMMYProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections;
    int i;

    if (flags & PROBE_DETECT)
	return FALSE;
    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(DUMMY_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }

    numUsed = numDevSections;

    if (numUsed > 0) {

	for (i = 0; i < numUsed; i++) {
	    ScrnInfoPtr pScrn = NULL;
	    int entityIndex = 
		xf86ClaimNoSlot(drv,DUMMY_CHIP,devSections[i],TRUE);
	    /* Allocate a ScrnInfoRec and claim the slot */
	    if ((pScrn = xf86AllocateScreen(drv,0 ))) {
		   xf86AddEntityToScreen(pScrn,entityIndex);
		    pScrn->driverVersion = DUMMY_VERSION;
		    pScrn->driverName    = DUMMY_DRIVER_NAME;
		    pScrn->name          = DUMMY_NAME;
		    pScrn->Probe         = DUMMYProbe;
		    pScrn->PreInit       = DUMMYPreInit;
		    pScrn->ScreenInit    = DUMMYScreenInit;
		    pScrn->SwitchMode    = DUMMYSwitchMode;
		    pScrn->AdjustFrame   = DUMMYAdjustFrame;
		    pScrn->EnterVT       = DUMMYEnterVT;
		    pScrn->LeaveVT       = DUMMYLeaveVT;
		    pScrn->FreeScreen    = DUMMYFreeScreen;
		    pScrn->ValidMode     = DUMMYValidMode;

		    foundScreen = TRUE;
	    }
	}
    }    
    return foundScreen;
}

# define RETURN \
    { DUMMYFreeRec(pScrn);\
			    return FALSE;\
					     }


#ifdef SPARKLE_MODE
//==================================================================================================

Bool DUMMYCrtc_resize(ScrnInfoPtr pScrn, int width, int height)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    DUMMYPtr dPtr = DUMMYPTR(pScrn);


    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Crtc resize: %dx%d\n", width, height);


    PixmapPtr pPixmap = pScreen->GetScreenPixmap(pScreen);

    if (pScrn->virtualX == width && pScrn->virtualY == height)
    {
        return TRUE;
    }

    pScrn->virtualX = width;
    pScrn->virtualY = height;
    pScrn->displayWidth = width; //FIXME

    int cpp = (pScrn->bitsPerPixel + 7) / 8;
    pScreen->ModifyPixmapHeader(pPixmap, width, height, -1, -1, pScrn->displayWidth * cpp, NULL);

    //FIXME Needed?
    int i;
    for (i = 0; i < xf86_config->num_crtc; i++)
    {
        xf86CrtcPtr crtc = xf86_config->crtc[i];

        if (!crtc->enabled)
            continue;

        DUMMYCrtc_set_mode_major(crtc, &crtc->mode, crtc->rotation, crtc->x, crtc->y);
    }

    dPtr->shared->pixmapWidth = pScrn->virtualX;
    dPtr->shared->pixmapHeight = pScrn->virtualY;

    return TRUE;
}

static const xf86CrtcConfigFuncsRec dummyCrtcConfigFuncs = {
    DUMMYCrtc_resize
    //Bool (*resize) (ScrnInfoPtr scrn, int width, int height);
};

//==================================================================================================

static void DUMMYCrtc_dpms(xf86CrtcPtr crtc, int mode)
{
}

//FIXME Check
static Bool DUMMYCrtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation, int x, int y)
{
    crtc->mode = *mode;
    crtc->x = x;
    crtc->y = y;
    crtc->rotation = rotation;

    return TRUE;
}

//FIXME Check mandatory
static const xf86CrtcFuncsRec dummyCrtcFuncs = {
    .dpms = DUMMYCrtc_dpms,
    .set_mode_major = DUMMYCrtc_set_mode_major,
    //void (*dpms) (xf86CrtcPtr crtc, int mode);
    //void (*save) (xf86CrtcPtr crtc);
    //void (*restore) (xf86CrtcPtr crtc);
    //Bool (*lock) (xf86CrtcPtr crtc);
    //void (*unlock) (xf86CrtcPtr crtc);
    //Bool (*mode_fixup) (xf86CrtcPtr crtc, DisplayModePtr mode, DisplayModePtr adjusted_mode);
    //void (*prepare) (xf86CrtcPtr crtc);
    //void (*mode_set) (xf86CrtcPtr crtc, DisplayModePtr mode, DisplayModePtr adjusted_mode, int x, int y);
    //void (*commit) (xf86CrtcPtr crtc);
    //void (*gamma_set) (xf86CrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue, int size);
    //void *(*shadow_allocate) (xf86CrtcPtr crtc, int width, int height);
    //PixmapPtr (*shadow_create) (xf86CrtcPtr crtc, void *data, int width, int height);
    //void (*shadow_destroy) (xf86CrtcPtr crtc, PixmapPtr pPixmap, void *data);
    //void (*set_cursor_colors) (xf86CrtcPtr crtc, int bg, int fg);
    //void (*set_cursor_position) (xf86CrtcPtr crtc, int x, int y);
    //void (*show_cursor) (xf86CrtcPtr crtc);
    //void (*hide_cursor) (xf86CrtcPtr crtc);
    //void (*load_cursor_image) (xf86CrtcPtr crtc, CARD8 *image);
    //Bool (*load_cursor_image_check) (xf86CrtcPtr crtc, CARD8 *image);
    //void (*load_cursor_argb) (xf86CrtcPtr crtc, CARD32 *image);
    //Bool (*load_cursor_argb_check) (xf86CrtcPtr crtc, CARD32 *image);
    //void (*destroy) (xf86CrtcPtr crtc);
    //Bool (*set_mode_major) (xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation, int x, int y);
    //void (*set_origin) (xf86CrtcPtr crtc, int x, int y);
    //Bool (*set_scanout_pixmap)(xf86CrtcPtr crtc, PixmapPtr pixmap);
};


//==================================================================================================


static void
DUMMYOutput_dmps(xf86OutputPtr output, int mode)
{
}

static xf86OutputStatus
DUMMYOutput_detect(xf86OutputPtr output)
{
    return XF86OutputStatusConnected;
}

static int
DUMMYOutput_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    //FIXME
    return MODE_OK;
}

static DisplayModePtr
DUMMYOutput_get_modes(xf86OutputPtr output)
{
    ScrnInfoPtr pScrn;
    DUMMYPtr dPtr;

    pScrn = output->scrn;
    dPtr = DUMMYPTR(pScrn);

    return xf86DuplicateModes(NULL, dPtr->modes);
}

static const xf86OutputFuncsRec dummyOutputFuncs = {
    .dpms = DUMMYOutput_dmps,
    .detect = DUMMYOutput_detect,
    .mode_valid = DUMMYOutput_mode_valid,
    .get_modes = DUMMYOutput_get_modes,
    //void (*create_resources) (xf86OutputPtr output);
    //void (*dpms) (xf86OutputPtr output, int mode);
    //void (*save) (xf86OutputPtr output);
    //void (*restore) (xf86OutputPtr output);
    //int (*mode_valid) (xf86OutputPtr output, DisplayModePtr pMode);
    //Bool (*mode_fixup) (xf86OutputPtr output, DisplayModePtr mode, DisplayModePtr adjusted_mode);
    //void (*prepare) (xf86OutputPtr output);
    //void (*commit) (xf86OutputPtr output);
    //void (*mode_set) (xf86OutputPtr output, DisplayModePtr mode, DisplayModePtr adjusted_mode);
    //xf86OutputStatus(*detect) (xf86OutputPtr output);
    //DisplayModePtr(*get_modes) (xf86OutputPtr output);
    //#ifdef RANDR_12_INTERFACE
    //Bool(*set_property) (xf86OutputPtr output, Atom property, RRPropertyValuePtr value);
    //#endif
    //#ifdef RANDR_13_INTERFACE
    //Bool (*get_property) (xf86OutputPtr output, Atom property);
    //#endif
    //#ifdef RANDR_GET_CRTC_INTERFACE
    //xf86CrtcPtr(*get_crtc) (xf86OutputPtr output);
    //#endif
    //void (*destroy) (xf86OutputPtr output);
};


//==================================================================================================
#endif

/* Mandatory */
Bool
DUMMYPreInit(ScrnInfoPtr pScrn, int flags)
{
    ClockRangePtr clockRanges;
    int i;
    DUMMYPtr dPtr;
    int maxClock = 230000;
    GDevPtr device = xf86GetEntityInfo(pScrn->entityList[0])->device;
#ifdef SPARKLE_MODE
    xf86CrtcPtr crtc;
    xf86OutputPtr output;
#endif

    if (flags & PROBE_DETECT) 
	return TRUE;
    
    /* Allocate the DummyRec driverPrivate */
    if (!DUMMYGetRec(pScrn)) {
	return FALSE;
    }
    
    dPtr = DUMMYPTR(pScrn);

    pScrn->chipset = (char *)xf86TokenToString(DUMMYChipsets,
					       DUMMY_CHIP);

#ifndef SPARKLE_MODE
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Chipset is a DUMMY\n");
#endif

    pScrn->monitor = pScrn->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrn, 0, 0, 0,  Support24bppFb | Support32bppFb))
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
#ifndef SPARKLE_MODE
	case 15:
#endif
	case 16:
	case 24:
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }

    xf86PrintDepthBpp(pScrn);
    if (pScrn->depth == 8)
	pScrn->rgbBits = 8;

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
	    ;
	}
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) 
	return FALSE;

    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    xf86CollectOptions(pScrn, device->options);
    /* Process the options */
    if (!(dPtr->Options = malloc(sizeof(DUMMYOptions))))
	return FALSE;
    memcpy(dPtr->Options, DUMMYOptions, sizeof(DUMMYOptions));

    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, dPtr->Options);

    xf86GetOptValBool(dPtr->Options, OPTION_SW_CURSOR,&dPtr->swCursor);

    if (device->videoRam != 0) {
	pScrn->videoRam = device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);
    } else {
#ifndef SPARKLE_MODE
	pScrn->videoRam = 4096;
#else
    pScrn->videoRam = 16 * 1096;
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);
    }
    
    if (device->dacSpeeds[0] != 0) {
	maxClock = device->dacSpeeds[0];
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Max Clock: %d kHz\n",
		   maxClock);
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Max Clock: %d kHz\n",
		   maxClock);
    }

    pScrn->progClock = TRUE;

#ifndef SPARKLE_MODE
    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfcalloc(sizeof(ClockRange), 1);
    clockRanges->next = NULL;
    clockRanges->ClockMulFactor = 1;
    clockRanges->minClock = 11000;   /* guessed §§§ */
    clockRanges->maxClock = 300000;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE; 
    clockRanges->doubleScanAllowed = TRUE;

    /* Subtract memory for HW cursor */


    {
	int apertureSize = (pScrn->videoRam * 1024);
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, DUMMY_MAX_WIDTH,
			      (8 * pScrn->bitsPerPixel),
			      128, DUMMY_MAX_HEIGHT, pScrn->display->virtualX,
			      pScrn->display->virtualY, apertureSize,
			      LOOKUP_BEST_REFRESH);

       if (i == -1)
           RETURN;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	RETURN;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.
     *
     * Calling this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set them.  They
     * are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, 0); 
 
    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

#else

    dPtr->modes = xf86CVTMode(800, 600, 60, 0, 0);

    //XXX Check results
    xf86CrtcConfigInit(pScrn, &dummyCrtcConfigFuncs);
    xf86CrtcSetSizeRange(pScrn, 240, 240, 2048, 2048);

    crtc = xf86CrtcCreate(pScrn, &dummyCrtcFuncs);
    output = xf86OutputCreate(pScrn, &dummyOutputFuncs, "sparkle");
    output->possible_crtcs = 0x7f; //Why?

    xf86InitialConfiguration (pScrn, TRUE);

    pScrn->currentMode = pScrn->modes; //Needed?

    crtc->funcs->set_mode_major(crtc, pScrn->currentMode, RR_Rotate_0, 0, 0); //Needed?

    xf86SetDpi(pScrn, 0, 0);

    //output->mm_width = 0;
    //output->mm_height = 0;
    
#endif

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	RETURN;
    }

    if (!dPtr->swCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac"))
	    RETURN;
    }
    
    /* We have no contiguous physical fb in physical memory */
    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    return TRUE;
}
#undef RETURN

/* Mandatory */
static Bool
DUMMYEnterVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    
    /* Should we re-save the text mode on each VT enter? */
    if(!dummyModeInit(pScrn, pScrn->currentMode))
      return FALSE;

    DUMMYAdjustFrame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));

    return TRUE;
}

/* Mandatory */
static void
DUMMYLeaveVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    dummyRestore(pScrn, TRUE);
}

static void
DUMMYLoadPalette(
   ScrnInfoPtr pScrn,
   int numColors,
   int *indices,
   LOCO *colors,
   VisualPtr pVisual
){
   int i, index, shift, Gshift;
   DUMMYPtr dPtr = DUMMYPTR(pScrn);

   switch(pScrn->depth) {
   case 15:	
	shift = Gshift = 1;
	break;
   case 16:
	shift = 0; 
        Gshift = 0;
	break;
   default:
	shift = Gshift = 0;
	break;
   }

   for(i = 0; i < numColors; i++) {
       index = indices[i];
       dPtr->colors[index].red = colors[index].red << shift;
       dPtr->colors[index].green = colors[index].green << Gshift;
       dPtr->colors[index].blue = colors[index].blue << shift;
   } 

}

static ScrnInfoPtr DUMMYScrn; /* static-globalize it */

/* Mandatory */
static Bool
DUMMYScreenInit(SCREEN_INIT_ARGS_DECL)
{
    ScrnInfoPtr pScrn;
    DUMMYPtr dPtr;
    int ret;
    VisualPtr visual;
    
    /*
     * we need to get the ScrnInfoRec for this screen, so let's allocate
     * one first thing
     */
    pScrn = xf86ScreenToScrn(pScreen);
    dPtr = DUMMYPTR(pScrn);
    DUMMYScrn = pScrn;


#ifndef SPARKLE_MODE
    if (!(dPtr->FBBase = malloc(pScrn->videoRam * 1024)))
	return FALSE;
#else
    if (DUMMYOpenSharedResources(pScreen) != TRUE)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to open shared resources\n");
        return FALSE;
    }

    dPtr->configuredWidth = 0;
    dPtr->configuredHeight = 0;
#endif
    
    /*
     * next we save the current state and setup the first mode
     */
    dummySave(pScrn);
    
    if (!dummyModeInit(pScrn,pScrn->currentMode))
	return FALSE;
    DUMMYAdjustFrame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));

    /*
     * Reset visual list.
     */
    miClearVisualTypes();
    
    /* Setup the visuals we support. */
    
    if (!miSetVisualTypes(pScrn->depth,
      		      miGetDefaultVisualMask(pScrn->depth),
		      pScrn->rgbBits, pScrn->defaultVisual))
         return FALSE;

    if (!miSetPixmapDepths ()) return FALSE;

#ifdef SPARKLE_MODE
    pScrn->displayWidth = pScrn->virtualX; //FIXME Segfault without this
#endif

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */
    ret = fbScreenInit(pScreen, dPtr->FBBase,
			    pScrn->virtualX, pScrn->virtualY,
			    pScrn->xDpi, pScrn->yDpi,
			    pScrn->displayWidth, pScrn->bitsPerPixel);
    if (!ret)
	return FALSE;

    if (pScrn->depth > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }
    
    /* must be after RGB ordering fixed */
    fbPictureInit(pScreen, 0, 0);

#ifdef SPARKLE_MODE
    dPtr->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = DUMMYCreateScreenResources;
#endif

    xf86SetBlackWhitePixels(pScreen);

#ifdef USE_DGA
    DUMMYDGAInit(pScreen);
#endif
    
    if (dPtr->swCursor)
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using Software Cursor.\n");

    {

	 
	BoxRec AvailFBArea;
	int lines = pScrn->videoRam * 1024 /
	    (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));
	AvailFBArea.x1 = 0;
	AvailFBArea.y1 = 0;
	AvailFBArea.x2 = pScrn->displayWidth;
	AvailFBArea.y2 = lines;
	xf86InitFBManager(pScreen, &AvailFBArea); 
	
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		   "Using %i scanlines of offscreen memory \n"
		   , lines - pScrn->virtualY);
    }

    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
	
    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());


    if (!dPtr->swCursor) {
      /* HW cursor functions */
      if (!DUMMYCursorInit(pScreen)) {
	  xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		     "Hardware cursor initialization failed\n");
	  return FALSE;
      }
    }
    
    /* Initialise default colourmap */
    if(!miCreateDefColormap(pScreen))
	return FALSE;

    if (!xf86HandleColormaps(pScreen, 256, pScrn->rgbBits,
                         DUMMYLoadPalette, NULL, 
                         CMAP_PALETTED_TRUECOLOR 
			     | CMAP_RELOAD_ON_MODE_SWITCH))
	return FALSE;

/*     DUMMYInitVideo(pScreen); */

    pScreen->SaveScreen = DUMMYSaveScreen;

    
    /* Wrap the current CloseScreen function */
    dPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = DUMMYCloseScreen;

#ifdef SPARKLE_MODE
    dPtr->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = DUMMYBlockHandler;
#endif

#ifdef SPARKLE_MODE
    //Bool xf86DiDGAInit (ScreenPtr screen, unsigned long dga_address);
    xf86CrtcScreenInit(pScreen); //FIXME Check result. See modesetting.
#endif

#ifndef SPARKLE_MODE
    /* Wrap the current CreateWindow function */
    dPtr->CreateWindow = pScreen->CreateWindow;
    pScreen->CreateWindow = DUMMYCreateWindow;
#endif

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    return TRUE;
}

/* Mandatory */
Bool
DUMMYSwitchMode(SWITCH_MODE_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
#ifdef SPARKLE_MODE
    //Bool xf86SetSingleMode (ScrnInfoPtr scrn, DisplayModePtr desired, Rotation rotation);
#endif
    return dummyModeInit(pScrn, mode);
}

/* Mandatory */
void
DUMMYAdjustFrame(ADJUST_FRAME_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    int Base; 

    Base = (y * pScrn->displayWidth + x) >> 2;

    /* Scale Base by the number of bytes per pixel. */
    switch (pScrn->depth) {
    case  8 :
	break;
    case 15 :
    case 16 :
	Base *= 2;
	break;
    case 24 :
	Base *= 3;
	break;
    default :
	break;
    }
}

/* Mandatory */
static Bool
DUMMYCloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DUMMYPtr dPtr = DUMMYPTR(pScrn);

    if(pScrn->vtSema){
 	dummyRestore(pScrn, TRUE);
#ifndef SPARKLE_MODE
	free(dPtr->FBBase);
#else
    DUMMYCloseSharedResources(pScreen);
#endif
    }

    if (dPtr->CursorInfo)
	xf86DestroyCursorInfoRec(dPtr->CursorInfo);

#ifdef SPARKLE_MODE
    DamageUnregister(dPtr->damage);
    DamageDestroy(dPtr->damage);
#endif

    pScrn->vtSema = FALSE;
#ifdef SPARKLE_MODE
    pScreen->CreateScreenResources = dPtr->CreateScreenResources;
    pScreen->BlockHandler = dPtr->BlockHandler;
#endif
    pScreen->CloseScreen = dPtr->CloseScreen;
    return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);
}

/* Optional */
static void
DUMMYFreeScreen(FREE_SCREEN_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    DUMMYFreeRec(pScrn);
}

static Bool
DUMMYSaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = NULL;
    DUMMYPtr dPtr;

    if (pScreen != NULL) {
	pScrn = xf86ScreenToScrn(pScreen);
	dPtr = DUMMYPTR(pScrn);

	dPtr->screenSaver = xf86IsUnblank(mode);
    } 
    return TRUE;
}

/* Optional */
static ModeStatus
DUMMYValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags)
{
    return(MODE_OK);
}

static void
dummySave(ScrnInfoPtr pScrn)
{
}

static void 
dummyRestore(ScrnInfoPtr pScrn, Bool restoreText)
{
}
    
static Bool
dummyModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    dummyRestore(pScrn, FALSE);
    
    return(TRUE);
}

Atom VFB_PROP  = 0;
#define  VFB_PROP_NAME  "VFB_IDENT"

#ifndef SPARKLE_MODE
static Bool
DUMMYCreateWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DUMMYPtr dPtr = DUMMYPTR(DUMMYScrn);
    WindowPtr pWinRoot;
    int ret;

    pScreen->CreateWindow = dPtr->CreateWindow;
    ret = pScreen->CreateWindow(pWin);
    dPtr->CreateWindow = pScreen->CreateWindow;
    pScreen->CreateWindow = DUMMYCreateWindow;

    if(ret != TRUE)
	return(ret);
	
    if(dPtr->prop == FALSE) {
#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 8
        pWinRoot = WindowTable[DUMMYScrn->pScreen->myNum];
#else
        pWinRoot = DUMMYScrn->pScreen->root;
#endif
        if (! ValidAtom(VFB_PROP))
            VFB_PROP = MakeAtom(VFB_PROP_NAME, strlen(VFB_PROP_NAME), 1);

        ret = ChangeWindowProperty(pWinRoot, VFB_PROP, XA_STRING, 
		8, PropModeReplace, (int)4, (pointer)"TRUE", FALSE);
	if( ret != Success)
		ErrorF("Could not set VFB root window property");
        dPtr->prop = TRUE;

	return TRUE;
    }
    return TRUE;
}
#endif

#ifndef HW_SKIP_CONSOLE
#define HW_SKIP_CONSOLE 4
#endif

static Bool
dummyDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op, pointer ptr)
{
    CARD32 *flag;
    
    switch (op) {
	case GET_REQUIRED_HW_INTERFACES:
	    flag = (CARD32*)ptr;
	    (*flag) = HW_SKIP_CONSOLE;
	    return TRUE;
	default:
	    return FALSE;
    }
}


#ifdef SPARKLE_MODE

static Bool
DUMMYUpdateModes(ScrnInfoPtr pScrn, int width, int height)
{
    DUMMYPtr dPtr;
    dPtr = DUMMYPTR(pScrn);

    DisplayModePtr mode = dPtr->modes;
    while (mode != NULL)
    {
        DisplayModePtr nextMode = mode->next;
        free((void *)mode->name);
        free(mode);
        mode = nextMode;
    }   
    dPtr->modes = NULL;

    dPtr->modes = xf86ModesAdd(dPtr->modes, xf86CVTMode(width, height, 60, 0, 0)); //Native

    dPtr->modes = xf86ModesAdd(dPtr->modes, xf86CVTMode(width*10/15, height*10/15, 60, 0, 0));
    dPtr->modes = xf86ModesAdd(dPtr->modes, xf86CVTMode(width/2, height/2, 60, 0, 0));
    dPtr->modes = xf86ModesAdd(dPtr->modes, xf86CVTMode(width/4, height/4, 60, 0, 0));

    dPtr->modes = xf86ModesAdd(dPtr->modes, xf86CVTMode(width*15/10, height*15/10, 60, 0, 0));

    xf86ProbeOutputModes(pScrn, 2048, 2048);
    xf86SetScrnInfoModes(pScrn); //Needed?

    return TRUE;
}

static Bool
DUMMYOpenSharedResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn;
    DUMMYPtr dPtr;

    pScrn = xf86ScreenToScrn(pScreen);
    dPtr = DUMMYPTR(pScrn);

    dPtr->shared_framebuffer = shared_resource_open("/dev/sparkle_framebuffer", pScrn->videoRam * 1024, 1, (void **)&dPtr->FBBase);
    if (dPtr->shared_framebuffer == NULL)
    {
        return FALSE;
    }

    dPtr->shared_info = shared_resource_open("/dev/sparkle_info", sizeof(struct sparkle_shared_t), 1, (void **)&dPtr->shared);
    if (dPtr->shared_info == NULL)
    {
        return FALSE;
    }

    //XXX Check
    dPtr->shared->pixmapWidth = pScrn->virtualX;
    dPtr->shared->pixmapHeight = pScrn->virtualY;

	switch (pScrn->bitsPerPixel)
    {
	case 8:
        dPtr->shared->pixmapFormat = SPARKLE_FORMAT_STATIC_GRAY;
        break;
	case 16:
        dPtr->shared->pixmapFormat = SPARKLE_FORMAT_RGB565;
        break;
	case 32:
        dPtr->shared->pixmapFormat = SPARKLE_FORMAT_BGRA8888;
	    break;
	default:
	    return FALSE;
	}

    dPtr->shared->surfaceWidth = 0;
    dPtr->shared->surfaceHeight = 0;
    dPtr->shared->damage = 0;
    dPtr->shared->damageX1 = 0;
    dPtr->shared->damageY1 = 0;
    dPtr->shared->damageX2 = 0;
    dPtr->shared->damageY2 = 0;

    return TRUE;
}

static Bool
DUMMYCloseSharedResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn;
    DUMMYPtr dPtr;

    pScrn = xf86ScreenToScrn(pScreen);
    dPtr = DUMMYPTR(pScrn);

    shared_resource_close(dPtr->shared_info);
    shared_resource_close(dPtr->shared_framebuffer);

    return TRUE;
}

static Bool
DUMMYCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DUMMYPtr dPtr = DUMMYPTR(pScrn);
    Bool ret;
    PixmapPtr pPixmap;

    pScreen->CreateScreenResources = dPtr->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = DUMMYCreateScreenResources;

    if (!ret)
	return FALSE;

    pPixmap = pScreen->GetScreenPixmap(pScreen);

    dPtr->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE, pScreen, pPixmap);
    if (!dPtr->damage)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to create screen damage record\n");
        return FALSE;
    }

    DamageRegister(&pPixmap->drawable, dPtr->damage);

    return TRUE;
}

static void
DUMMYBlockHandler(BLOCKHANDLER_ARGS_DECL)
{
    SCREEN_PTR(arg);

    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DUMMYPtr dPtr = DUMMYPTR(pScrn);

    pScreen->BlockHandler = dPtr->BlockHandler;
    pScreen->BlockHandler(BLOCKHANDLER_ARGS);
    dPtr->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = DUMMYBlockHandler;


    //FIXME
    {
        if (dPtr->shared->surfaceWidth != dPtr->configuredWidth || dPtr->shared->surfaceHeight != dPtr->configuredHeight)
        {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Reconfiguring for %dx%d\n", dPtr->shared->surfaceWidth, dPtr->shared->surfaceHeight);

            DUMMYUpdateModes(pScrn, dPtr->shared->surfaceWidth, dPtr->shared->surfaceHeight);

            DisplayModePtr mode = dPtr->modes;
            //pScrn->currentMode = mode;

            //FIXME Take DPI from config
            int mmWidth = mode->HDisplay * 254 / 960;
            int mmHeight = mode->VDisplay * 254 / 960;

            RRScreenSizeSet(pScreen, mode->HDisplay, mode->VDisplay, mmWidth, mmHeight);
            xf86SetSingleMode(pScrn, mode, RR_Rotate_0);

            dPtr->configuredWidth = dPtr->shared->surfaceWidth;
            dPtr->configuredHeight = dPtr->shared->surfaceHeight;
        }
    }


    RegionPtr pRegion = DamageRegion(dPtr->damage);

    //FIXME Shared mutex?
    if (RegionNotEmpty(pRegion))
    {
        if (dPtr->shared->damage == 0)
        {
            dPtr->shared->damageX1 = pRegion->extents.x1;
            dPtr->shared->damageY1 = pRegion->extents.y1;
            dPtr->shared->damageX2 = pRegion->extents.x2;
            dPtr->shared->damageY2 = pRegion->extents.y2;
            dPtr->shared->damage = 1;
        }
        else
        {
            dPtr->shared->damageX1 = min(pRegion->extents.x1, dPtr->shared->damageX1);
            dPtr->shared->damageY1 = min(pRegion->extents.y1, dPtr->shared->damageY1);
            dPtr->shared->damageX2 = max(pRegion->extents.x2, dPtr->shared->damageX2);
            dPtr->shared->damageY2 = max(pRegion->extents.y2, dPtr->shared->damageY2);
        }

        DamageEmpty(dPtr->damage);
    }
}


#endif

