/*
 * ComboBoxEx control v2 (mod3)
 *
 * Copyright 1998, 1999 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 *
 * FIXME:
 *   - should include "combo.h" 

 * Changes  Guy Albertelli <galberte@neo.lrun.com>
 * v1  Implemented messages: CB_SETITEMHEIGHT, WM_WINDOWPOSCHANGING,
 *      WM_DRAWITEM, and WM_MEASUREITEM. Fixed WM_CREATE. Fixed height
 *      of window rect reported fixing rebar control.
 * v2
 *   1. Rewrite of WM_Create for own created EDIT control. Also try to
 *      generate message sequence similar to native DLL.
 *   2. Handle case where CBEM_SETITEM is called to display data in EDIT
 *      Control. 
 *   3. Add override for WNDPROC for the EDIT control (reqed for VK_RETURN).
 *   4. Dump input data for things using COMBOBOXEXITEM{A|W}.
 *   5. Handle positioning EDIT control based on whether icon present.
 *   6. Make InsertItemA use InsertItemW, and store all data in ..W form.
 *   7. Implement CBEM_DELETEITEM, CBEM_GETITEM{A|W}, CB_SETCURSEL,
 *      CBEM_{GET|SET}UNICODEFORMAT.
 *   8. Add override for WNDPROC for the COMBO control.
 *   9. Support extended style CBES_EX_NOEDITIMAGE and warn others are not
 *      supported.
 *  10. Implement CB_FINDSTRINGEXACT in both the Combo and ComboEx window
 *      procs to match the items. This eliminates dup entries in the listbox.
 *
 * Test vehicals were the ControlSpy modules (rebar.exe and comboboxex.exe),
 *  and IE 4.0.
 *
 */

#include "winbase.h"
#include "commctrl.h"
#include "debugtools.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(comboex);
/*
 * The following is necessary for the test done in COMBOEX_DrawItem
 * to determine whether to dump out the DRAWITEM structure or not.
 */
DECLARE_DEBUG_CHANNEL(message);

/* Item structure */
typedef struct
{
    VOID         *next;
    UINT         mask;
    LPWSTR       pszText;
    int          cchTextMax;
    int          iImage;
    int          iSelectedImage;
    int          iOverlay;
    int          iIndent;
    LPARAM       lParam;
} CBE_ITEMDATA;

/* ComboBoxEx structure */
typedef struct
{
    HIMAGELIST   himl;
    HWND         hwndSelf;         /* my own hwnd */
    HWND         hwndCombo;
    HWND         hwndEdit;
    WNDPROC      prevEditWndProc;  /* previous Edit WNDPROC value */
    WNDPROC      prevComboWndProc;   /* previous Combo WNDPROC value */
    DWORD        dwExtStyle;
    DWORD        flags;            /* WINE internal flags */
    HFONT        font;
    INT          nb_items;         /* Number of items */
    BOOL         bUnicode;         /* ASCII (FALSE) or Unicode (TRUE)? */
    CBE_ITEMDATA *edit;            /* item data for edit item */
    CBE_ITEMDATA *items;           /* Array of items */
} COMBOEX_INFO;

/* internal flags in the COMBOEX_INFO structure */
#define  WCBE_ACTEDIT        0x00000001     /* Edit active i.e.
                                             * CBEN_BEGINEDIT issued
                                             * but CBEN_ENDEDIT{A|W}
                                             * not yet issued. */
#define  WCBE_EDITCHG        0x00000002     /* Edit issued EN_CHANGE */


#define ID_CB_EDIT    1001


/*
 * Special flag set in DRAWITEMSTRUCT itemState field. It is set by
 * the ComboEx version of the Combo Window Proc so that when the 
 * WM_DRAWITEM message is then passed to ComboEx, we know that this 
 * particular WM_DRAWITEM message is for listbox only items. Any messasges
 * without this flag is then for the Edit control field.
 *
 * We really cannot use the ODS_COMBOBOXEDIT flag because MSDN states that 
 * only version 4.0 applications will have ODS_COMBOBOXEDIT set.
 */
#define ODS_COMBOEXLBOX  0x4000



/* Height in pixels of control over the amount of the selected font */
#define CBE_EXTRA     3

/* Indent amount per MS documentation */
#define CBE_INDENT    10

/* Offset in pixels from left side for start of image or text */
#define CBE_STARTOFFSET   6

/* Offset between image and text */
#define CBE_SEP   4

#define COMBOEX_GetInfoPtr(hwnd) ((COMBOEX_INFO *)GetWindowLongA (hwnd, 0))


/* Things common to the entire DLL */
static ATOM ComboExInfo;
static LRESULT WINAPI
COMBOEX_EditWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI
COMBOEX_ComboWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void
COMBOEX_DumpItem (CBE_ITEMDATA *item)
{
    if (TRACE_ON(comboex)){
      TRACE("item %p - mask=%08x, pszText=%p, cchTM=%d, iImage=%d\n",
	    item, item->mask, item->pszText, item->cchTextMax,
	    item->iImage);
      TRACE("item %p - iSelectedImage=%d, iOverlay=%d, iIndent=%d, lParam=%08lx\n",
	    item, item->iSelectedImage, item->iOverlay, item->iIndent, item->lParam);
      if ((item->mask & CBEIF_TEXT) && item->pszText)
	  TRACE("item %p - pszText=%s\n",
		item, debugstr_w((const WCHAR *)item->pszText));
    }
}


static void
COMBOEX_DumpInput (COMBOBOXEXITEMA *input, BOOL true_for_w)
{
    if (TRACE_ON(comboex)){
      TRACE("input - mask=%08x, iItem=%d, pszText=%p, cchTM=%d, iImage=%d\n",
	    input->mask, input->iItem, input->pszText, input->cchTextMax,
	    input->iImage);
      if ((input->mask & CBEIF_TEXT) && input->pszText) {
	  if (true_for_w)
	      TRACE("input - pszText=<%s>\n", 
		    debugstr_w((const WCHAR *)input->pszText));
	  else 
	      TRACE("input - pszText=<%s>\n", 
		    debugstr_a((const char *)input->pszText));
      }
      TRACE("input - iSelectedImage=%d, iOverlay=%d, iIndent=%d, lParam=%08lx\n",
	    input->iSelectedImage, input->iOverlay, input->iIndent, input->lParam);
    }
}


inline static LRESULT
COMBOEX_Forward (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if (infoPtr->hwndCombo)    
	return SendMessageA (infoPtr->hwndCombo, uMsg, wParam, lParam);

    return 0;
}


static INT
COMBOEX_Notify (COMBOEX_INFO *infoPtr, INT code, NMHDR *hdr, BOOL doW)
{

    hdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    hdr->hwndFrom = infoPtr->hwndSelf;
    hdr->code = code;
    if (doW)
	return SendMessageW (GetParent(infoPtr->hwndSelf), WM_NOTIFY, 0,
			     (LPARAM)hdr);
    else
	return SendMessageA (GetParent(infoPtr->hwndSelf), WM_NOTIFY, 0,
			     (LPARAM)hdr);
}


static void
COMBOEX_GetComboFontSize (COMBOEX_INFO *infoPtr, SIZE *size)
{
    HFONT nfont, ofont;
    HDC mydc;

    mydc = GetDC (0); /* why the entire screen???? */
    nfont = SendMessageW (infoPtr->hwndCombo, WM_GETFONT, 0, 0);
    ofont = (HFONT) SelectObject (mydc, nfont);
    GetTextExtentPointA (mydc, "A", 1, size);
    SelectObject (mydc, ofont);
    ReleaseDC (0, mydc);
    TRACE("selected font hwnd=%08x, height=%ld\n", nfont, size->cy);
}


static void
COMBOEX_CopyItem (COMBOEX_INFO *infoPtr, CBE_ITEMDATA *item, COMBOBOXEXITEMW *cit)
{
    if (cit->mask & CBEIF_TEXT) {
        cit->pszText        = item->pszText;
        cit->cchTextMax     = item->cchTextMax;
    }
    if (cit->mask & CBEIF_IMAGE)
	cit->iImage         = item->iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
	cit->iSelectedImage = item->iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
	cit->iOverlay       = item->iOverlay;
    if (cit->mask & CBEIF_INDENT)
	cit->iIndent        = item->iIndent;
    if (cit->mask & CBEIF_LPARAM)
	cit->lParam         = item->lParam;

}


static void
COMBOEX_AdjustEditPos (COMBOEX_INFO *infoPtr)
{
    SIZE mysize;
    IMAGEINFO iinfo;
    INT x, y, w, h, xoff = 0;
    RECT rect;

    if (!infoPtr->hwndEdit) return;
    iinfo.rcImage.left = iinfo.rcImage.right = 0;
    if (infoPtr->himl) {
	ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo);
	xoff = iinfo.rcImage.right - iinfo.rcImage.left + CBE_SEP;
    }
    GetClientRect (infoPtr->hwndCombo, &rect);
    InflateRect (&rect, -2, -2);
    InvalidateRect (infoPtr->hwndCombo, &rect, TRUE);

    /* reposition the Edit control based on whether icon exists */
    COMBOEX_GetComboFontSize (infoPtr, &mysize);
    TRACE("Combo font x=%ld, y=%ld\n", mysize.cx, mysize.cy);
    x = xoff + CBE_STARTOFFSET + 1;
    y = CBE_EXTRA + 1;
    w = rect.right-rect.left - x - GetSystemMetrics(SM_CXVSCROLL) - 1;
    h = mysize.cy + 1;

    TRACE("Combo client (%d,%d)-(%d,%d), setting Edit to (%d,%d)-(%d,%d)\n",
	  rect.left, rect.top, rect.right, rect.bottom,
	  x, y, x + w, y + h);
    SetWindowPos(infoPtr->hwndEdit, HWND_TOP,
		 x, y,
		 w, h,
		 SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
}


static void
COMBOEX_ReSize (HWND hwnd, COMBOEX_INFO *infoPtr)
{
    SIZE mysize;
    UINT cy;
    IMAGEINFO iinfo;

    COMBOEX_GetComboFontSize (infoPtr, &mysize);
    cy = mysize.cy + CBE_EXTRA;
    if (infoPtr->himl) {
	ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo);
	cy = max (iinfo.rcImage.bottom - iinfo.rcImage.top, cy);
	TRACE("upgraded height due to image:  height=%d\n", cy);
    }
    SendMessageW (hwnd, CB_SETITEMHEIGHT, (WPARAM) -1, (LPARAM) cy);
    if (infoPtr->hwndCombo)
        SendMessageW (infoPtr->hwndCombo, CB_SETITEMHEIGHT,
		      (WPARAM) 0, (LPARAM) cy);
}


static void
COMBOEX_SetEditText (COMBOEX_INFO *infoPtr, CBE_ITEMDATA *item)
{
    if (!infoPtr->hwndEdit) return;
    /* native issues the following messages to the {Edit} control */
    /*      WM_SETTEXT (0,addr)     */
    /*      EM_SETSEL32 (0,0)       */
    /*      EM_SETSEL32 (0,-1)      */
    if (item->mask & CBEIF_TEXT) {
	SendMessageW (infoPtr->hwndEdit, WM_SETTEXT, 0, (LPARAM)item->pszText);
	SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, 0);
	SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, -1);
    }
}

 
static CBE_ITEMDATA *
COMBOEX_FindItem(COMBOEX_INFO *infoPtr, INT index)
{
    CBE_ITEMDATA *item;
    INT i;

    if ((index > infoPtr->nb_items) || (index < -1))
	return 0;
    if (index == -1)
	return infoPtr->edit;
    item = infoPtr->items;
    i = infoPtr->nb_items - 1;

    /* find the item in the list */
    while (item && (i > index)) {
	item = (CBE_ITEMDATA *)item->next;
	i--;
    }
    if (!item || (i != index)) {
	FIXME("COMBOBOXEX item structures broken. Please report!\n");
	return 0;
    }
    return item;
}


/* ***  CBEM_xxx message support  *** */


static LRESULT
COMBOEX_DeleteItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    INT index = (INT) wParam;
    CBE_ITEMDATA *item;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    /* if item number requested does not exist then return failure */
    if ((index > infoPtr->nb_items) || (index < 0)) {
	ERR("attempt to delete item that does not exist\n");
	return CB_ERR;
    }

    if (!(item = COMBOEX_FindItem(infoPtr, index))) {
	ERR("attempt to delete item that was not found!\n");
	return CB_ERR;
    }

    /* doing this will result in WM_DELETEITEM being issued */
    SendMessageW (infoPtr->hwndCombo, CB_DELETESTRING, (WPARAM)index, 0);

    return infoPtr->nb_items;
}


inline static LRESULT
COMBOEX_GetComboControl (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("\n");

    return (LRESULT)infoPtr->hwndCombo;
}


inline static LRESULT
COMBOEX_GetEditControl (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if ((GetWindowLongA (hwnd, GWL_STYLE) & CBS_DROPDOWNLIST) != CBS_DROPDOWN)
	return 0;

    TRACE("-- 0x%x\n", infoPtr->hwndEdit);

    return (LRESULT)infoPtr->hwndEdit;
}


inline static LRESULT
COMBOEX_GetExtendedStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->dwExtStyle;
}


inline static LRESULT
COMBOEX_GetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    return (LRESULT)infoPtr->himl;
}


static LRESULT
COMBOEX_GetItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMW *cit = (COMBOBOXEXITEMW *) lParam;
    INT index;
    CBE_ITEMDATA *item;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    /* get real index of item to insert */
    index = cit->iItem;

    /* if item number requested does not exist then return failure */
    if ((index > infoPtr->nb_items) || (index < -1)) {
	ERR("attempt to get item that does not exist\n");
	return 0;
    }

    /* if the item is the edit control and there is no edit control, skip */
    if ((index == -1) && 
        ((GetWindowLongA (hwnd, GWL_STYLE) & CBS_DROPDOWNLIST) != CBS_DROPDOWN))
	    return 0;

    if (!(item = COMBOEX_FindItem(infoPtr, index))) {
	ERR("attempt to get item that was not found!\n");
	return 0;
    }

    COMBOEX_CopyItem (infoPtr, item, cit);

    return TRUE;
}


inline static LRESULT
COMBOEX_GetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOBOXEXITEMA *cit = (COMBOBOXEXITEMA *) lParam;
    COMBOBOXEXITEMW tmpcit;
    INT len;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    tmpcit.mask = cit->mask;
    tmpcit.iItem = cit->iItem;
    COMBOEX_GetItemW (hwnd, wParam, (LPARAM) &tmpcit);

    len = WideCharToMultiByte (CP_ACP, 0, tmpcit.pszText, -1, 0, 0, NULL, NULL);
    if (len > 0) 
	WideCharToMultiByte (CP_ACP, 0, tmpcit.pszText, -1, 
			     cit->pszText, cit->cchTextMax, NULL, NULL);

    cit->iImage = tmpcit.iImage;
    cit->iSelectedImage = tmpcit.iSelectedImage;
    cit->iOverlay = tmpcit.iOverlay;
    cit->iIndent = tmpcit.iIndent;
    cit->lParam = tmpcit.lParam;

    return TRUE;
}


inline static LRESULT
COMBOEX_GetUnicodeFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("%s hwnd=0x%x stub!\n", 
	   infoPtr->bUnicode ? "TRUE" : "FALSE", hwnd);

    return infoPtr->bUnicode;
}


inline static LRESULT
COMBOEX_HasEditChanged (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if ((GetWindowLongA (hwnd, GWL_STYLE) & CBS_DROPDOWNLIST) != CBS_DROPDOWN)
	return FALSE;
    if ((infoPtr->flags & (WCBE_ACTEDIT | WCBE_EDITCHG)) == 
	(WCBE_ACTEDIT | WCBE_EDITCHG))
	return TRUE;
    return FALSE;
}


static LRESULT
COMBOEX_InsertItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMW *cit = (COMBOBOXEXITEMW *) lParam;
    INT index;
    CBE_ITEMDATA *item;
    NMCOMBOBOXEXW nmcit;

    TRACE("\n");

    COMBOEX_DumpInput ((COMBOBOXEXITEMA *) cit, TRUE);

    /* get real index of item to insert */
    index = cit->iItem;
    if (index == -1) index = infoPtr->nb_items;
    if (index > infoPtr->nb_items) index = infoPtr->nb_items;

    /* get space and chain it in */
    item = (CBE_ITEMDATA *)COMCTL32_Alloc (sizeof (CBE_ITEMDATA));
    item->next = NULL;
    item->pszText = NULL;

    /* locate position to insert new item in */
    if (index == infoPtr->nb_items) {
        /* fast path for iItem = -1 */
        item->next = infoPtr->items;
	infoPtr->items = item;
    }
    else {
        INT i = infoPtr->nb_items-1;
	CBE_ITEMDATA *moving = infoPtr->items;

	while ((i > index) && moving) {
	    moving = (CBE_ITEMDATA *)moving->next;
	    i--;
	}
	if (!moving) {
	    FIXME("COMBOBOXEX item structures broken. Please report!\n");
	    COMCTL32_Free(item);
	    return -1;
	}
	item->next = moving->next;
	moving->next = item;
    }

    /* fill in our hidden item structure */
    item->mask           = cit->mask;
    if (item->mask & CBEIF_TEXT) {
        LPWSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str = (LPWSTR) L"";
	len = strlenW (str);
	if (len > 0) {
	    item->pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (item->pszText, str);
	}
        item->cchTextMax   = cit->cchTextMax;
    }
    if (item->mask & CBEIF_IMAGE)
      item->iImage         = cit->iImage;
    if (item->mask & CBEIF_SELECTEDIMAGE)
      item->iSelectedImage = cit->iSelectedImage;
    if (item->mask & CBEIF_OVERLAY)
      item->iOverlay       = cit->iOverlay;
    if (item->mask & CBEIF_INDENT)
      item->iIndent        = cit->iIndent;
    if (item->mask & CBEIF_LPARAM)
      item->lParam         = cit->lParam;
    infoPtr->nb_items++;

    COMBOEX_DumpItem (item);

    SendMessageW (infoPtr->hwndCombo, CB_INSERTSTRING, 
		  (WPARAM)cit->iItem, (LPARAM)item);

    COMBOEX_CopyItem (infoPtr, item, &nmcit.ceItem);
    COMBOEX_Notify (infoPtr, CBEN_INSERTITEM, (NMHDR *)&nmcit, TRUE);

    return index;

}


static LRESULT
COMBOEX_InsertItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOBOXEXITEMA	*cit = (COMBOBOXEXITEMA *) lParam;
    COMBOBOXEXITEMW	citW;
    LRESULT		ret;

    memcpy(&citW,cit,sizeof(COMBOBOXEXITEMA));
    if (cit->mask & CBEIF_TEXT) {
        LPSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str="";
	len = MultiByteToWideChar (CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0) {
	    citW.pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    MultiByteToWideChar (CP_ACP, 0, str, -1, citW.pszText, len);
	}
    }
    ret = COMBOEX_InsertItemW(hwnd,wParam,(LPARAM)&citW);;

    if (cit->mask & CBEIF_TEXT)
	COMCTL32_Free(citW.pszText);
    return ret;
}


static LRESULT
COMBOEX_SetExtendedStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    DWORD dwTemp;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    dwTemp = infoPtr->dwExtStyle;

    if (lParam & (CBES_EX_NOEDITIMAGEINDENT |
		  CBES_EX_PATHWORDBREAKPROC |
		  CBES_EX_NOSIZELIMIT |
		  CBES_EX_CASESENSITIVE))
	FIXME("Extended style not implemented %08lx\n", lParam);

    if ((DWORD)wParam) {
	infoPtr->dwExtStyle = (infoPtr->dwExtStyle & ~(DWORD)wParam) | (DWORD)lParam;
    }
    else
	infoPtr->dwExtStyle = (DWORD)lParam;

    /*
     * native does this for CBES_EX_NOEDITIMAGE state change
     */
    if ((infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGE) ^
	(dwTemp & CBES_EX_NOEDITIMAGE)) {
	/* if state of EX_NOEDITIMAGE changes, invalidate all */
	TRACE("EX_NOEDITIMAGE state changed to %ld\n",
	    infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGE);
	InvalidateRect (hwnd, NULL, TRUE);
	COMBOEX_AdjustEditPos (infoPtr);
	if (infoPtr->hwndEdit)
	    InvalidateRect (infoPtr->hwndEdit, NULL, TRUE);
    }

    return (LRESULT)dwTemp;
}


inline static LRESULT
COMBOEX_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    HIMAGELIST himlTemp;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    himlTemp = infoPtr->himl;
    infoPtr->himl = (HIMAGELIST)lParam;

    COMBOEX_ReSize (hwnd, infoPtr);
    InvalidateRect (infoPtr->hwndCombo, NULL, TRUE);

    /* reposition the Edit control based on whether icon exists */
    COMBOEX_AdjustEditPos (infoPtr);
    return (LRESULT)himlTemp;
}

static LRESULT
COMBOEX_SetItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMW *cit = (COMBOBOXEXITEMW *) lParam;
    INT index;
    CBE_ITEMDATA *item;

    COMBOEX_DumpInput ((COMBOBOXEXITEMA *) cit, TRUE);

    /* get real index of item to insert */
    index = cit->iItem;

    /* if item number requested does not exist then return failure */
    if ((index > infoPtr->nb_items) || (index < -1)) {
	ERR("attempt to set item that does not exist yet!\n");
	return 0;
    }

    /* if the item is the edit control and there is no edit control, skip */
    if ((index == -1) && 
        ((GetWindowLongA (hwnd, GWL_STYLE) & CBS_DROPDOWNLIST) != CBS_DROPDOWN))
	    return 0;

    if (!(item = COMBOEX_FindItem(infoPtr, index))) {
	ERR("attempt to set item that was not found!\n");
	return 0;
    }

    /* add/change stuff to the internal item structure */ 
    item->mask |= cit->mask;
    if (cit->mask & CBEIF_TEXT) {
        LPWSTR str;
	INT len;
	WCHAR emptystr[1] = {0};

        str = cit->pszText;
        if (!str) str=emptystr;
	len = strlenW(str);
	if (len > 0) {
	    item->pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW(item->pszText,str);
	}
        item->cchTextMax   = cit->cchTextMax;
    }
    if (cit->mask & CBEIF_IMAGE)
      item->iImage         = cit->iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
      item->iSelectedImage = cit->iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
      item->iOverlay       = cit->iOverlay;
    if (cit->mask & CBEIF_INDENT)
      item->iIndent        = cit->iIndent;
    if (cit->mask & CBEIF_LPARAM)
      cit->lParam         = cit->lParam;

    COMBOEX_DumpItem (item);

    /* if original request was to update edit control, do some fast foot work */
    if (cit->iItem == -1) {
	COMBOEX_SetEditText (infoPtr, item);
	RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE | RDW_INVALIDATE);
    }
    return TRUE;
}

static LRESULT
COMBOEX_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOBOXEXITEMA	*cit = (COMBOBOXEXITEMA *) lParam;
    COMBOBOXEXITEMW	citW;
    LRESULT		ret;

    memcpy(&citW,cit,sizeof(COMBOBOXEXITEMA));
    if (cit->mask & CBEIF_TEXT) {
        LPSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str="";
	len = MultiByteToWideChar (CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0) {
	    citW.pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    MultiByteToWideChar (CP_ACP, 0, str, -1, citW.pszText, len);
	}
    }
    ret = COMBOEX_SetItemW(hwnd,wParam,(LPARAM)&citW);;

    if (cit->mask & CBEIF_TEXT)
	COMCTL32_Free(citW.pszText);
    return ret;
}


inline static LRESULT
COMBOEX_SetUnicodeFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    BOOL bTemp;

    TRACE("%s hwnd=0x%04x stub!\n", 
	   ((BOOL)wParam) ? "TRUE" : "FALSE", hwnd);

    bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}



/* ***  CB_xxx message support  *** */


static LRESULT
COMBOEX_FindStringExact (COMBOEX_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT i, count;
    CBE_ITEMDATA *item;
    LPWSTR desired = NULL;
    INT start = (INT) wParam;

    i = MultiByteToWideChar (CP_ACP, 0, (LPSTR)lParam, -1, NULL, 0);
    if (i > 0) {
	desired = (LPWSTR)COMCTL32_Alloc ((i + 1)*sizeof(WCHAR));
	MultiByteToWideChar (CP_ACP, 0, (LPSTR)lParam, -1, desired, i);
    }

    count = SendMessageW (infoPtr->hwndCombo, CB_GETCOUNT, 0 , 0);

    /* now search from after starting loc and wrapping back to start */
    for(i=start+1; i<count; i++) {
	item = (CBE_ITEMDATA *)SendMessageW (infoPtr->hwndCombo, 
			  CB_GETITEMDATA, (WPARAM)i, 0);
	TRACE("desired=%s, item=%s\n", 
	      debugstr_w(desired), debugstr_w(item->pszText));
	if (lstrcmpiW(item->pszText, desired) == 0) {
	    COMCTL32_Free (desired);
	    return i;
	}
    }
    for(i=0; i<=start; i++) {
	item = (CBE_ITEMDATA *)SendMessageW (infoPtr->hwndCombo, 
			  CB_GETITEMDATA, (WPARAM)i, 0);
	TRACE("desired=%s, item=%s\n", 
	      debugstr_w(desired), debugstr_w(item->pszText));
	if (lstrcmpiW(item->pszText, desired) == 0) {
	    COMCTL32_Free (desired);
	    return i;
	}
    }
    COMCTL32_Free(desired);
    return CB_ERR;
}


static LRESULT
COMBOEX_SetCursel (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT index = wParam;
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    CBE_ITEMDATA *item;
    LRESULT lret;

    if (!(item = COMBOEX_FindItem(infoPtr, index))) {
	/* FIXME: need to clear selection */
	return CB_ERR;
    }

    TRACE("selecting item %d text=%s\n", index, (item->pszText) ?
	  debugstr_w(item->pszText) : "<null>");

    lret = SendMessageW (infoPtr->hwndCombo, CB_SETCURSEL, wParam, lParam);
    COMBOEX_SetEditText (infoPtr, item);
    return lret;
}


static LRESULT
COMBOEX_SetItemHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    RECT cb_wrect, cbx_wrect, cbx_crect;
    LRESULT ret = 0;
    UINT height;

    /* First, lets forward the message to the normal combo control
       just like Windows.     */
    if (infoPtr->hwndCombo)    
       SendMessageW (infoPtr->hwndCombo, CB_SETITEMHEIGHT, wParam, lParam);

    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);
    GetWindowRect (hwnd, &cbx_wrect);
    GetClientRect (hwnd, &cbx_crect);
    /* the height of comboex as height of the combo + comboex border */ 
    height = cb_wrect.bottom-cb_wrect.top
             + cbx_wrect.bottom-cbx_wrect.top
             - (cbx_crect.bottom-cbx_crect.top);
    TRACE("EX window=(%d,%d)-(%d,%d), client=(%d,%d)-(%d,%d)\n",
	  cbx_wrect.left, cbx_wrect.top, cbx_wrect.right, cbx_wrect.bottom,
	  cbx_crect.left, cbx_crect.top, cbx_crect.right, cbx_crect.bottom);
    TRACE("CB window=(%d,%d)-(%d,%d), EX setting=(0,0)-(%d,%d)\n",
	  cb_wrect.left, cb_wrect.top, cb_wrect.right, cb_wrect.bottom,
	  cbx_wrect.right-cbx_wrect.left, height);
    SetWindowPos (hwnd, HWND_TOP, 0, 0,
		  cbx_wrect.right-cbx_wrect.left,
		  height,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

    return ret;
}


/* ***  WM_xxx message support  *** */


static LRESULT
COMBOEX_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA) lParam;
    COMBOEX_INFO *infoPtr;
    DWORD dwComboStyle;
    LOGFONTA mylogfont;
    CBE_ITEMDATA *item;
    RECT wnrc1, clrc1, cmbwrc;
    LONG test;

    /* allocate memory for info structure */
    infoPtr = (COMBOEX_INFO *)COMCTL32_Alloc (sizeof(COMBOEX_INFO));
    if (infoPtr == NULL) {
	ERR("could not allocate info memory!\n");
	return 0;
    }

    /* initialize info structure */

    infoPtr->items    = NULL;
    infoPtr->nb_items = 0;
    infoPtr->hwndSelf = hwnd;

    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* create combo box */
    dwComboStyle = GetWindowLongA (hwnd, GWL_STYLE) &
			(CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|WS_CHILD);

    TRACE("combo style=%08lx, additional style=%08lx\n", dwComboStyle,
          WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL |
          CBS_NOINTEGRALHEIGHT | CBS_DROPDOWNLIST |
	  WS_CHILD | WS_VISIBLE | CBS_OWNERDRAWFIXED);

    /* Native version of ComboEx creates the ComboBox with DROPDOWNLIST */
    /* specified. It then creates it's own version of the EDIT control  */
    /* and makes the ComboBox the parent. This is because a normal      */
    /* DROPDOWNLIST does not have a EDIT control, but we need one.      */
    /* We also need to place the edit control at the proper location    */
    /* (allow space for the icons).                                     */

    infoPtr->hwndCombo = CreateWindowA ("ComboBox", "",
			 /* following line added to match native */
                         WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL |
                         CBS_NOINTEGRALHEIGHT | CBS_DROPDOWNLIST | 
			 /* was base and is necessary */
			 WS_CHILD | WS_VISIBLE | CBS_OWNERDRAWFIXED | dwComboStyle,
			 cs->y, cs->x, cs->cx, cs->cy, hwnd,
			 (HMENU) GetWindowLongA (hwnd, GWL_ID),
			 GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);

    /* 
     * native does the following at this point according to trace:
     *  GetWindowThreadProcessId(hwndCombo,0)
     *  GetCurrentThreadId()
     *  GetWindowThreadProcessId(hwndCombo, &???)
     *  GetCurrentProcessId()
     */

    /*
     * Setup a property to hold the pointer to the COMBOBOXEX 
     * data structure.
     */
    test = GetPropA(infoPtr->hwndCombo, (LPCSTR)(LONG)ComboExInfo);
    if (!test || ((COMBOEX_INFO *)test != infoPtr)) {
	SetPropA(infoPtr->hwndCombo, "CC32SubclassInfo", (LONG)infoPtr);
    }
    infoPtr->prevComboWndProc = (WNDPROC)SetWindowLongA(infoPtr->hwndCombo, 
	                        GWL_WNDPROC, (LONG)COMBOEX_ComboWndProc);
    infoPtr->font = SendMessageW (infoPtr->hwndCombo, WM_GETFONT, 0, 0);


    /*
     * Now create our own EDIT control so we can position it.
     * It is created only for CBS_DROPDOWN style
     */
    if ((cs->style & CBS_DROPDOWNLIST) == CBS_DROPDOWN) {
	infoPtr->hwndEdit = CreateWindowExA (0, "EDIT", "",
		    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ES_AUTOHSCROLL,
		    0, 0, 0, 0,  /* will set later */
		    infoPtr->hwndCombo, 
		    (HMENU) GetWindowLongA (hwnd, GWL_ID),
		    GetWindowLongA (hwnd, GWL_HINSTANCE),
		    NULL);

	/* native does the following at this point according to trace:
	 *  GetWindowThreadProcessId(hwndEdit,0)
	 *  GetCurrentThreadId()
	 *  GetWindowThreadProcessId(hwndEdit, &???)
	 *  GetCurrentProcessId()
	 */

	/*
	 * Setup a property to hold the pointer to the COMBOBOXEX 
	 * data structure.
	 */
	test = GetPropA(infoPtr->hwndEdit, (LPCSTR)(LONG)ComboExInfo);
	if (!test || ((COMBOEX_INFO *)test != infoPtr)) {
	    SetPropA(infoPtr->hwndEdit, "CC32SubclassInfo", (LONG)infoPtr);
	}
	infoPtr->prevEditWndProc = (WNDPROC)SetWindowLongA(infoPtr->hwndEdit, 
				 GWL_WNDPROC, (LONG)COMBOEX_EditWndProc);
	infoPtr->font = SendMessageW (infoPtr->hwndCombo, WM_GETFONT, 0, 0);
    }
    else {
	infoPtr->hwndEdit = 0;
	infoPtr->font = 0;
    }

    /*
     * Locate the default font if necessary and then set it in
     * all associated controls
     */
    if (!infoPtr->font) {
	SystemParametersInfoA (SPI_GETICONTITLELOGFONT, sizeof(mylogfont), 
			       &mylogfont, 0);
	infoPtr->font = CreateFontIndirectA (&mylogfont);
    }
    SendMessageW (infoPtr->hwndCombo, WM_SETFONT, (WPARAM)infoPtr->font, 0);
    if (infoPtr->hwndEdit) {
	SendMessageW (infoPtr->hwndEdit, WM_SETFONT, (WPARAM)infoPtr->font, 0);
	SendMessageW (infoPtr->hwndEdit, EM_SETMARGINS, (WPARAM)EC_USEFONTINFO, 0);
    }

    COMBOEX_ReSize (hwnd, infoPtr);

    /* Above is fairly certain, below is much less certain. */

    GetWindowRect(hwnd, &wnrc1);
    GetClientRect(hwnd, &clrc1);
    GetWindowRect(infoPtr->hwndCombo, &cmbwrc);
    TRACE("Ex wnd=(%d,%d)-(%d,%d) Ex clt=(%d,%d)-(%d,%d) Cb wnd=(%d,%d)-(%d,%d)\n",
	  wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	  clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	  cmbwrc.left, cmbwrc.top, cmbwrc.right, cmbwrc.bottom);
    SetWindowPos(infoPtr->hwndCombo, HWND_TOP, 
		 0, 0, wnrc1.right-wnrc1.left, wnrc1.bottom-wnrc1.top,
		 SWP_NOACTIVATE | SWP_NOREDRAW);

    GetWindowRect(infoPtr->hwndCombo, &cmbwrc);
    TRACE("Ex wnd=(%d,%d)-(%d,%d)\n",
	  cmbwrc.left, cmbwrc.top, cmbwrc.right, cmbwrc.bottom);
    SetWindowPos(hwnd, HWND_TOP, 
		 0, 0, cmbwrc.right-cmbwrc.left, cmbwrc.bottom-cmbwrc.top,
		 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

    COMBOEX_AdjustEditPos (infoPtr);

    /*
     * Create an item structure to represent the data in the 
     * EDIT control.
     */
    item = (CBE_ITEMDATA *)COMCTL32_Alloc (sizeof (CBE_ITEMDATA));
    item->next = NULL;
    item->pszText = NULL;
    item->mask = 0;
    infoPtr->edit = item;

    return 0;
}


inline static LRESULT
COMBOEX_Command (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LRESULT lret;
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    INT command = HIWORD(wParam);
    CBE_ITEMDATA *item = 0;
    WCHAR wintext[520];
    INT cursel, n;
    NMCBEENDEDITA cbeend;

    TRACE("for command %d\n", command);

    switch (command)
    {
    case CBN_DROPDOWN:
	SendMessageW (GetParent (hwnd), WM_COMMAND, wParam, 
			     (LPARAM)hwnd);
	/*
	 * from native trace of first dropdown after typing in URL in IE4 
	 *  CB_GETCURSEL(Combo)
	 *  GetWindowText(Edit)
	 *  CB_GETCURSEL(Combo)
	 *  CB_GETCOUNT(Combo)
	 *  CB_GETITEMDATA(Combo, n)
	 *  WM_NOTIFY(parent, CBEN_ENDEDITA|W)
	 *  CB_GETCURSEL(Combo)
	 *  CB_SETCURSEL(COMBOEX, n)
	 *  SetFocus(Combo)
	 * the rest is supposition  
	 */
	cursel = SendMessageW (infoPtr->hwndCombo, CB_GETCURSEL, 0, 0);
	if (cursel == -1) {
	    /* find match from edit against those in Combobox */
	    GetWindowTextW (infoPtr->hwndEdit, wintext, 520);
	    n = SendMessageW (infoPtr->hwndCombo, CB_GETCOUNT, 0, 0);
	    for (cursel = 0; cursel < n; cursel++){
		item = (CBE_ITEMDATA *)SendMessageW (infoPtr->hwndCombo, 
						     CB_GETITEMDATA, 
						     cursel, 0);
		if ((INT)item == CB_ERR) break;
		if (lstrcmpiW(item->pszText, wintext) == 0) break;
	    }
	    if ((cursel == n) || ((INT)item == CB_ERR)) {
		TRACE("failed to find match??? item=%p cursel=%d\n",
		      item, cursel);
		if (infoPtr->hwndEdit)
		    SetFocus(infoPtr->hwndEdit);
		return 0;
	    }
	}
	if (infoPtr->flags & WCBE_ACTEDIT) {
	    cbeend.fChanged = (infoPtr->flags & WCBE_EDITCHG);
	    cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						 CB_GETCURSEL, 0, 0);
	    cbeend.iWhy = CBENF_DROPDOWN;

	    infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
	    if (COMBOEX_Notify (infoPtr, CBEN_ENDEDITA, 
				(NMHDR *)&cbeend, FALSE)) {
		/* abort the change */
		TRACE("Notify requested abort of change\n");
		return 0;
	    }
	}
	SendMessageW (hwnd, CB_SETCURSEL, cursel, 0);
	SetFocus(infoPtr->hwndCombo);
	return 0;

    default:
	/* 
	 * We have to change the handle since we are the control
	 * issuing the message. IE4 depends on this.
	 * We also need to set the focus back to the Edit control
	 * after passing the command to the parent of the ComboEx.
	 */
	lret = SendMessageW (GetParent (hwnd), WM_COMMAND, wParam, 
			     (LPARAM)hwnd);
	if (infoPtr->hwndEdit)
	    SetFocus(infoPtr->hwndEdit);
	return lret;
    }
    return 0;
}


inline static LRESULT
COMBOEX_WM_DeleteItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    DELETEITEMSTRUCT *dis = (DELETEITEMSTRUCT *)lParam;
    CBE_ITEMDATA *item, *olditem;
    INT i;
    NMCOMBOBOXEXW nmcit;

    TRACE("CtlType=%08x, CtlID=%08x, itemID=%08x, hwnd=%x, data=%08lx\n",
	  dis->CtlType, dis->CtlID, dis->itemID, dis->hwndItem, dis->itemData);

    if ((dis->itemID >= infoPtr->nb_items) || (dis->itemID < 0)) return FALSE;

    olditem = infoPtr->items;
    i = infoPtr->nb_items - 1;

    if (i == dis->itemID) {
	infoPtr->items = infoPtr->items->next;
    }
    else {
	item = olditem;
	i--;

	/* find the prior item in the list */
	while (item->next && (i > dis->itemID)) {
	    item = (CBE_ITEMDATA *)item->next;
	    i--;
	}
	if (!item->next || (i != dis->itemID)) {
	    FIXME("COMBOBOXEX item structures broken. Please report!\n");
	    return FALSE;
	}
	olditem = item->next;
	item->next = (CBE_ITEMDATA *)((CBE_ITEMDATA *)item->next)->next;
    }
    infoPtr->nb_items--;

    COMBOEX_CopyItem (infoPtr, olditem, &nmcit.ceItem);
    COMBOEX_Notify (infoPtr, CBEN_DELETEITEM, (NMHDR *)&nmcit, TRUE);

    COMCTL32_Free(olditem);

    return TRUE;

}


inline static LRESULT
COMBOEX_DrawItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
    CBE_ITEMDATA *item = 0;
    SIZE txtsize;
    RECT rect;
    int drawimage;
    UINT xbase;
    UINT xioff = 0;               /* size and spacer of image if any */
    IMAGEINFO iinfo;
    INT len;

    if (!IsWindowEnabled(infoPtr->hwndCombo)) return 0;

    /* MSDN says:                                                       */
    /*     "itemID - Specifies the menu item identifier for a menu      */
    /*      item or the index of the item in a list box or combo box.   */
    /*      For an empty list box or combo box, this member can be -1.  */
    /*      This allows the application to draw only the focus          */
    /*      rectangle at the coordinates specified by the rcItem        */
    /*      member even though there are no items in the control.       */
    /*      This indicates to the user whether the list box or combo    */
    /*      box has the focus. How the bits are set in the itemAction   */
    /*      member determines whether the rectangle is to be drawn as   */
    /*      though the list box or combo box has the focus.             */
    if (dis->itemID == 0xffffffff) {
	if ( ( (dis->itemAction & ODA_FOCUS) && (dis->itemState & ODS_SELECTED)) ||
	     ( (dis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) && (dis->itemState & ODS_FOCUS) ) ) { 

	    TRACE("drawing item -1 special focus, rect=(%d,%d)-(%d,%d)\n",
		  dis->rcItem.left, dis->rcItem.top,
		  dis->rcItem.right, dis->rcItem.bottom);
	}
	else if ((dis->CtlType == ODT_COMBOBOX) && 
		 (dis->itemAction == ODA_DRAWENTIRE)) {
	    /* draw of edit control data */

	    /* testing */
	    {
		RECT exrc, cbrc, edrc;
		GetWindowRect (hwnd, &exrc);
		GetWindowRect (infoPtr->hwndCombo, &cbrc);
		edrc.left=edrc.top=edrc.right=edrc.bottom=-1;
		if (infoPtr->hwndEdit)
		    GetWindowRect (infoPtr->hwndEdit, &edrc);
		TRACE("window rects ex=(%d,%d)-(%d,%d), cb=(%d,%d)-(%d,%d), ed=(%d,%d)-(%d,%d)\n",
		      exrc.left, exrc.top, exrc.right, exrc.bottom,
		      cbrc.left, cbrc.top, cbrc.right, cbrc.bottom,
		      edrc.left, edrc.top, edrc.right, edrc.bottom);
	    }
	}
	else {
	    ERR("NOT drawing item  -1 special focus, rect=(%d,%d)-(%d,%d), action=%08x, state=%08x\n",
		dis->rcItem.left, dis->rcItem.top,
		dis->rcItem.right, dis->rcItem.bottom,
		dis->itemAction, dis->itemState);
	    return 0;
	}
    }

    /* If draw item is -1 (edit control) setup the item pointer */
    if (dis->itemID == 0xffffffff) {
	CHAR str[260];
	INT wlen, alen;

	if (!infoPtr->hwndEdit) {
	    ERR("request to draw edit item, but no edit control exists!\n");
	    return 0;
	}

	item = infoPtr->edit;
	/* free previous text of edit item */
	if (item->pszText) {
	    COMCTL32_Free(item->pszText);
	    item->pszText = 0;
	    item->mask &= ~CBEIF_TEXT;
	}
	alen = SendMessageA (infoPtr->hwndEdit, WM_GETTEXT, 260, (LPARAM)&str);
	TRACE("edit control hwndEdit=%0x, text len=%d str=<%s>\n",
	      infoPtr->hwndEdit, alen, str);
	if (alen > 0) {
	    item->mask |= CBEIF_TEXT;
	    wlen = MultiByteToWideChar (CP_ACP, 0, str, -1, NULL, 0);
	    if (wlen > 0) {
		item->pszText = (LPWSTR)COMCTL32_Alloc ((wlen + 1)*sizeof(WCHAR));
		MultiByteToWideChar (CP_ACP, 0, str, -1, item->pszText, wlen);
	    }
	}
    }

    /* if the item pointer is not set, then get the data and locate it */
    if (!item) {
	item = (CBE_ITEMDATA *)SendMessageW (infoPtr->hwndCombo,
			     CB_GETITEMDATA, (WPARAM)dis->itemID, 0);
	if (item == (CBE_ITEMDATA *)CB_ERR)
	    {
		FIXME("invalid item for id %d \n",dis->itemID);
		return 0;
	    }
    }

    /* dump the DRAWITEMSTRUCT if tracing "comboex" but not "message" */
    if (!TRACE_ON(message)) {
	TRACE("DRAWITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n", 
	      dis->CtlType, dis->CtlID);
	TRACE("itemID=0x%08x itemAction=0x%08x itemState=0x%08x\n", 
	      dis->itemID, dis->itemAction, dis->itemState);
	TRACE("hWnd=0x%04x hDC=0x%04x (%d,%d)-(%d,%d) itemData=0x%08lx\n",
	      dis->hwndItem, dis->hDC, dis->rcItem.left, 
	      dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 
	      dis->itemData);
    }
    COMBOEX_DumpItem (item);

    xbase = CBE_STARTOFFSET;
    if ((item->mask & CBEIF_INDENT) && (dis->itemState & ODS_COMBOEXLBOX))
        xbase += (item->iIndent * CBE_INDENT);
    if (item->mask & CBEIF_IMAGE) {
	ImageList_GetImageInfo(infoPtr->himl, item->iImage, &iinfo);
	xioff = (iinfo.rcImage.right - iinfo.rcImage.left + CBE_SEP);
    }

    switch (dis->itemAction) {
    case ODA_FOCUS:
        if (dis->itemState & ODS_SELECTED /*1*/) {
	    if ((item->mask & CBEIF_TEXT) && item->pszText) {
		RECT rect2;

	        len = strlenW (item->pszText);
		GetTextExtentPointW (dis->hDC, item->pszText, len, &txtsize);
		rect.left = xbase + xioff - 1;
		rect.right = rect.left + txtsize.cx + 2;
		rect.top = dis->rcItem.top;
		rect.bottom = dis->rcItem.bottom;
		GetClipBox (dis->hDC, &rect2);
		TRACE("drawing item %d focus, rect=(%d,%d)-(%d,%d)\n",
		      dis->itemID, rect.left, rect.top,
		      rect.right, rect.bottom);
		TRACE("                      clip=(%d,%d)-(%d,%d)\n",
		      rect2.left, rect2.top,
		      rect2.right, rect2.bottom);

		DrawFocusRect(dis->hDC, &rect);
	    }
	    else {
		FIXME("ODA_FOCUS and ODS_SELECTED but no text\n");
	    }
	}
	else {
	    FIXME("ODA_FOCUS but not ODS_SELECTED\n");
	}
        break;
    case ODA_SELECT:
    case ODA_DRAWENTIRE:
        drawimage = -1;
	if (!(infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGE)) {
	    if (item->mask & CBEIF_IMAGE) drawimage = item->iImage;
	    if ((dis->itemState & ODS_SELECTED) && 
		(item->mask & CBEIF_SELECTEDIMAGE))
	              drawimage = item->iSelectedImage;
	}
	if (drawimage != -1) {
	    TRACE("drawing image state=%d\n", dis->itemState & ODS_SELECTED);
	    ImageList_Draw (infoPtr->himl, drawimage, dis->hDC, 
			    xbase, dis->rcItem.top, 
			    (dis->itemState & ODS_SELECTED) ? 
			    ILD_SELECTED : ILD_NORMAL);
	}
	if ((item->mask & CBEIF_TEXT) && item->pszText) { 
	    UINT x, y;
	    COLORREF nbkc, ntxc;

	    len = lstrlenW (item->pszText);
	    GetTextExtentPointW (dis->hDC, item->pszText, len, &txtsize);
	    nbkc = GetSysColor ((dis->itemState & ODS_SELECTED) ?
			        COLOR_HIGHLIGHT : COLOR_WINDOW);
	    SetBkColor (dis->hDC, nbkc);
	    ntxc = GetSysColor ((dis->itemState & ODS_SELECTED) ?
			        COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	    SetTextColor (dis->hDC, ntxc);
	    x = xbase + xioff;
	    y = dis->rcItem.top +
	        (dis->rcItem.bottom - dis->rcItem.top - txtsize.cy) / 2;
	    rect.left = x;
	    rect.right = x + txtsize.cx;
	    rect.top = dis->rcItem.top + 1;
	    rect.bottom = dis->rcItem.bottom - 1;
	    TRACE("drawing item %d text, rect=(%d,%d)-(%d,%d)\n",
		  dis->itemID, rect.left, rect.top, rect.right, rect.bottom);
	    ExtTextOutW (dis->hDC, x, y, ETO_OPAQUE | ETO_CLIPPED,
			 &rect, item->pszText, len, 0);
	    if (dis->itemState & ODS_FOCUS) {
		rect.top -= 1;
		rect.bottom += 1;
		rect.left -= 1;
		rect.right += 1;
		TRACE("drawing item %d focus after text, rect=(%d,%d)-(%d,%d)\n",
		      dis->itemID, rect.left, rect.top, rect.right, rect.bottom);
		DrawFocusRect (dis->hDC, &rect);
	    }
	}
	break;
    default:
        FIXME("unknown action hwnd=%08x, wparam=%08x, lparam=%08lx, action=%d\n", 
	      hwnd, wParam, lParam, dis->itemAction);
    }

    return 0;
}


static LRESULT
COMBOEX_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if (infoPtr->hwndCombo)
	DestroyWindow (infoPtr->hwndCombo);

    if (infoPtr->edit) {
	COMCTL32_Free (infoPtr->edit);
	infoPtr->edit = 0;
    }

    if (infoPtr->items) {
        CBE_ITEMDATA *this, *next;

	this = infoPtr->items;
	while (this) {
	    next = (CBE_ITEMDATA *)this->next;
	    if ((this->mask & CBEIF_TEXT) && this->pszText)
	        COMCTL32_Free (this->pszText);
	    COMCTL32_Free (this);
	    this = next;
	}
    }

    DeleteObject (infoPtr->font);

    /* free comboex info data */
    COMCTL32_Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);
    return 0;
}


static LRESULT
COMBOEX_MeasureItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /*COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);*/
    MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
    HDC hdc;
    SIZE mysize;

    hdc = GetDC (0);
    GetTextExtentPointA (hdc, "W", 1, &mysize);
    ReleaseDC (0, hdc);
    mis->itemHeight = mysize.cy + CBE_EXTRA;

    TRACE("adjusted height hwnd=%08x, height=%d\n",
	  hwnd, mis->itemHeight);

    return 0;
}


static LRESULT
COMBOEX_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    RECT rect;

    GetWindowRect (hwnd, &rect);
    TRACE("my rect (%d,%d)-(%d,%d)\n",
	  rect.left, rect.top, rect.right, rect.bottom);

    MoveWindow (infoPtr->hwndCombo, 0, 0, rect.right -rect.left,
		  rect.bottom - rect.top, TRUE);

    COMBOEX_AdjustEditPos (infoPtr);

    return 0;
}


static LRESULT
COMBOEX_WindowPosChanging (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    LRESULT ret;
    RECT cbx_wrect, cbx_crect, cb_wrect;
    UINT width;
    WINDOWPOS *wp = (WINDOWPOS *)lParam;

    ret = DefWindowProcA (hwnd, WM_WINDOWPOSCHANGING, wParam, lParam);
    GetWindowRect (hwnd, &cbx_wrect);
    GetClientRect (hwnd, &cbx_crect);
    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);

    /* width is winpos value + border width of comboex */
    width = wp->cx
            + cbx_wrect.right-cbx_wrect.left 
            - (cbx_crect.right - cbx_crect.left); 

    TRACE("EX window=(%d,%d)-(%d,%d), client=(%d,%d)-(%d,%d)\n",
	  cbx_wrect.left, cbx_wrect.top, cbx_wrect.right, cbx_wrect.bottom,
	  cbx_crect.left, cbx_crect.top, cbx_crect.right, cbx_crect.bottom);
    TRACE("CB window=(%d,%d)-(%d,%d), EX setting=(0,0)-(%d,%d)\n",
	  cb_wrect.left, cb_wrect.top, cb_wrect.right, cb_wrect.bottom,
	  width, cb_wrect.bottom-cb_wrect.top);

    SetWindowPos (infoPtr->hwndCombo, HWND_TOP, 0, 0,
		  width,
		  cb_wrect.bottom-cb_wrect.top,
		  SWP_NOACTIVATE);

    COMBOEX_AdjustEditPos (infoPtr);

    return 0;
}


static LRESULT WINAPI
COMBOEX_EditWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = (COMBOEX_INFO *)GetPropA (hwnd, (LPCSTR)(LONG) ComboExInfo);
    NMCBEENDEDITA cbeend;
    COLORREF nbkc, obkc;
    HDC hDC;
    RECT rect;

    TRACE("hwnd=%x msg=%x wparam=%x lParam=%lx, info_ptr=%p\n", 
	  hwnd, uMsg, wParam, lParam, infoPtr);

    if (!infoPtr) return 0;

    switch (uMsg)
    {

    case WM_CHAR:
	    /* handle (ignore) the return character */
	    if (wParam == VK_RETURN) return 0;
	    /* all other characters pass into the real Edit */
	    return CallWindowProcA (infoPtr->prevEditWndProc, 
				   hwnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	    /*
	     * The following was determined by traces of the native 
	     */
            hDC = (HDC) wParam;
	    nbkc = GetSysColor (COLOR_WINDOW);
	    obkc = SetBkColor (hDC, nbkc);
            GetClientRect (hwnd, &rect);
	    TRACE("erasing (%d,%d)-(%d,%d)\n",
		  rect.left, rect.top, rect.right, rect.bottom);
	    ExtTextOutW (hDC, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
            SetBkColor (hDC, obkc);
	    return CallWindowProcA (infoPtr->prevEditWndProc, 
				   hwnd, uMsg, wParam, lParam);

    case WM_KEYDOWN: {
	    INT oldItem, selected;
	    CBE_ITEMDATA *item;
	    WCHAR edit_text[260];

	    switch ((INT)wParam)
	    {
	    case VK_ESCAPE:
		/* native version seems to do following for COMBOEX */
		/*
		 *   GetWindowTextA(Edit,&?, 0x104)             x
		 *   CB_GETCURSEL to Combo rets -1              x
		 *   WM_NOTIFY to COMBOEX parent (rebar)        x
		 *     (CBEN_ENDEDIT{A|W} 
		 *      fChanged = FALSE                        x
		 *      inewSelection = -1                      x
		 *      txt="www.hoho"                          x
		 *      iWhy = 3                                x
		 *   CB_GETCURSEL to Combo rets -1              x
		 *   InvalidateRect(Combo, 0)                   x
		 *   WM_SETTEXT to Edit                         x
		 *   EM_SETSEL to Edit (0,0)                    x
		 *   EM_SETSEL to Edit (0,-1)                   x
		 *   RedrawWindow(Combo, 0, 0, 5)               x
		 */
		TRACE("special code for VK_ESCAPE\n");

		GetWindowTextA (infoPtr->hwndEdit, cbeend.szText, 260);

		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		cbeend.fChanged = FALSE;
		cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						     CB_GETCURSEL, 0, 0);
		cbeend.iWhy = CBENF_ESCAPE;

		if (COMBOEX_Notify (infoPtr, CBEN_ENDEDITA, 
				    (NMHDR *)&cbeend, FALSE)) {
		    /* abort the change */
		    TRACE("Notify requested abort of change\n");
		    return 0;
		}
		oldItem = SendMessageW (infoPtr->hwndCombo,CB_GETCURSEL, 0, 0);
		InvalidateRect (infoPtr->hwndCombo, 0, 0);
		if (!(item = COMBOEX_FindItem(infoPtr, oldItem))) {
		    ERR("item %d not found. Problem!\n", oldItem);
		    break;
		}
		  
		COMBOEX_SetEditText (infoPtr, item);
		RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE | 
			      RDW_INVALIDATE);
		break;

	    case VK_RETURN:
		/* native version seems to do following for COMBOEX */
		/*
		 *   GetWindowTextA(Edit,&?, 0x104)             x
		 *   CB_GETCURSEL to Combo  rets -1             x
		 *   CB_GETCOUNT to Combo  rets 0
		 *   if >0 loop 
		 *       CB_GETITEMDATA to match
		 * *** above 3 lines simulated by FindItem      x  
		 *   WM_NOTIFY to COMBOEX parent (rebar)        x
		 *     (CBEN_ENDEDIT{A|W}                       x
		 *        fChanged = TRUE (-1)                  x
		 *        iNewSelection = -1 or selected        x
		 *        txt=                                  x
		 *        iWhy = 2 (CBENF_RETURN)               x
		 *   CB_GETCURSEL to Combo  rets -1             x
		 *   if -1 send CB_SETCURSEL to Combo -1        x
		 *   InvalidateRect(Combo, 0, 0)                x
		 *   SetFocus(Edit)                             x
		 *   CallWindowProc(406615a8, Edit, 0x100, 0xd, 0x1c0001)
		 */

		TRACE("special code for VK_RETURN\n");

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);

		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		selected = SendMessageW (infoPtr->hwndCombo,
					 CB_GETCURSEL, 0, 0);

		if (selected != -1) {
		    item = COMBOEX_FindItem (infoPtr, selected);
		    TRACE("handling VK_RETURN, selected = %d, selected_text=%s\n",
			  selected, debugstr_w(item->pszText));
		    TRACE("handling VK_RETURN, edittext=%s\n",
			  debugstr_w(edit_text));
		    if (lstrcmpiW (item->pszText, edit_text)) {
			/* strings not equal -- indicate edit has changed */
			selected = -1;
		    }
		}

		cbeend.iNewSelection = selected;
		cbeend.fChanged = TRUE; 
		cbeend.iWhy = CBENF_RETURN;
		WideCharToMultiByte (CP_ACP, 0, edit_text, -1,
				     cbeend.szText, sizeof(cbeend.szText),
				     NULL, NULL);

		if (COMBOEX_Notify (infoPtr, CBEN_ENDEDITA, 
				    (NMHDR *)&cbeend, FALSE)) {
		    /* abort the change, restore previous */
		    TRACE("Notify requested abort of change\n");
		    COMBOEX_SetEditText (infoPtr, infoPtr->edit);
		    RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE | 
				  RDW_INVALIDATE);
		    return 0;
		}
		oldItem = SendMessageW (infoPtr->hwndCombo,CB_GETCURSEL, 0, 0);
		if (oldItem != -1) {
		    /* if something is selected, then deselect it */
		    SendMessageW (infoPtr->hwndCombo, CB_SETCURSEL, 
				  (WPARAM)-1, 0);
		}
		InvalidateRect (infoPtr->hwndCombo, 0, 0);
		SetFocus(infoPtr->hwndEdit);
		break;

	    default:
		return CallWindowProcA (infoPtr->prevEditWndProc,
				       hwnd, uMsg, wParam, lParam);
	    }
	    return 0;
            }

    case WM_KILLFOCUS:
	    /* 
	     * should do NOTIFY CBEN_ENDEDIT with CBENF_KILLFOCUS
	     */
	    if (infoPtr->flags & WCBE_ACTEDIT) {
		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		cbeend.fChanged = FALSE;
		cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						     CB_GETCURSEL, 0, 0);
		cbeend.iWhy = CBENF_KILLFOCUS;

		COMBOEX_Notify (infoPtr, CBEN_ENDEDITA, 
				(NMHDR *)&cbeend, FALSE);
	    }
	    /* fall through */

    default:
	    return CallWindowProcA (infoPtr->prevEditWndProc, 
				   hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


static LRESULT WINAPI
COMBOEX_ComboWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = (COMBOEX_INFO *)GetPropA (hwnd, (LPCSTR)(LONG) ComboExInfo);
    NMMOUSE nmmse;
    COLORREF nbkc, obkc;
    HDC hDC;
    RECT rect;

    TRACE("hwnd=%x msg=%x wparam=%x lParam=%lx, info_ptr=%p\n", 
	  hwnd, uMsg, wParam, lParam, infoPtr);

    if (!infoPtr) return 0;

    switch (uMsg)
    {

    case CB_FINDSTRINGEXACT:
	    return COMBOEX_FindStringExact (infoPtr, wParam, lParam);

    case WM_DRAWITEM:
	    /*
	     * The only way this message should come is from the
	     * child Listbox issuing the message. Flag this so
	     * that ComboEx knows this is listbox.
	     */
	    ((DRAWITEMSTRUCT *)lParam)->itemState |= ODS_COMBOEXLBOX;
	    return CallWindowProcA (infoPtr->prevComboWndProc, 
				   hwnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	    /*
	     * The following was determined by traces of the native 
	     */
            hDC = (HDC) wParam;
	    nbkc = GetSysColor (COLOR_WINDOW);
	    obkc = SetBkColor (hDC, nbkc);
            GetClientRect (hwnd, &rect);
	    TRACE("erasing (%d,%d)-(%d,%d)\n",
		  rect.left, rect.top, rect.right, rect.bottom);
	    ExtTextOutW (hDC, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
            SetBkColor (hDC, obkc);
	    return CallWindowProcA (infoPtr->prevComboWndProc, 
				   hwnd, uMsg, wParam, lParam);

    case WM_SETCURSOR:
	    /*
	     *  WM_NOTIFY to comboex parent (rebar)
	     *   with NM_SETCURSOR with extra words of 0,0,0,0,0x02010001
	     *  CallWindowProc (previous)
	     */
	    nmmse.dwItemSpec = 0;
	    nmmse.dwItemData = 0;
	    nmmse.pt.x = 0;
	    nmmse.pt.y = 0;
	    nmmse.dwHitInfo = lParam;
	    COMBOEX_Notify (infoPtr, NM_SETCURSOR, (NMHDR *)&nmmse, FALSE);
	    return CallWindowProcA (infoPtr->prevComboWndProc, 
				   hwnd, uMsg, wParam, lParam);

    case WM_COMMAND:
	    /* traces show that COMBOEX does not issue CBN_EDITUPDATE
	     * on the EN_UPDATE
	     */
	    if (HIWORD(wParam) == EN_UPDATE) return 0;

	    if (HIWORD(wParam) == EN_SETFOCUS) {
		/* 
		 * For EN_SETFOCUS this issues the same calls and messages
		 *  as the native seems to do.
		 *
		 * for some cases however native does the following:
		 *   (noticed after SetFocus during LBUTTONDOWN on
		 *    on dropdown arrow)
		 *  WM_GETTEXTLENGTH (Edit);
		 *  WM_GETTEXT (Edit, len+1, str);
		 *  EM_SETSEL (Edit, 0, 0);
		 *  WM_GETTEXTLENGTH (Edit);
		 *  WM_GETTEXT (Edit, len+1, str);
		 *  EM_SETSEL (Edit, 0, len);
		 *  WM_NOTIFY (parent, CBEN_BEGINEDIT)
		 */
		NMHDR hdr;

		SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, 0);
		SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, -1);
		COMBOEX_Notify (infoPtr, CBEN_BEGINEDIT, &hdr, FALSE);
		infoPtr->flags |= WCBE_ACTEDIT;
		infoPtr->flags &= ~WCBE_EDITCHG; /* no change yet */
		return 0;
	    }

	    if (HIWORD(wParam) == EN_CHANGE) {
		/* 
		 * For EN_CHANGE this issues the same calls and messages
		 *  as the native seems to do.
		 */
		WCHAR edit_text[260];
		WCHAR *lastwrk;
		INT selected, cnt;
		CBE_ITEMDATA *item;

		selected = SendMessageW (infoPtr->hwndCombo,
					 CB_GETCURSEL, 0, 0);

		/* lstrlenA( lastworkingURL ) */

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);
		if (selected == -1) {
		    lastwrk = infoPtr->edit->pszText;
		    cnt = lstrlenW (lastwrk);
		    if (cnt >= 259) cnt = 259;
		}
		else {
		    item = COMBOEX_FindItem (infoPtr, selected);
		    cnt = lstrlenW (item->pszText);
		    lastwrk = item->pszText;
		    if (cnt >= 259) cnt = 259;
		}

		TRACE("handling EN_CHANGE, selected = %d, selected_text=%s\n",
		    selected, debugstr_w(lastwrk));
		TRACE("handling EN_CHANGE, edittext=%s\n",
		      debugstr_w(edit_text));

		/* lstrcmpiW is between lastworkingURL and GetWindowText */

		if (lstrcmpiW (lastwrk, edit_text)) {
		    /* strings not equal -- indicate edit has changed */
		    infoPtr->flags |= WCBE_EDITCHG;
		}
		SendMessageW ( GetParent(infoPtr->hwndSelf), WM_COMMAND,
			       MAKEWPARAM(GetDlgCtrlID (infoPtr->hwndSelf),
					  CBN_EDITCHANGE),
			       infoPtr->hwndSelf);
		return 0;
	    }

	    /* fall through */

    default:
	    return CallWindowProcA (infoPtr->prevComboWndProc, 
				   hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


static LRESULT WINAPI
COMBOEX_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%x msg=%x wparam=%x lParam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!COMBOEX_GetInfoPtr (hwnd) && (uMsg != WM_CREATE))
        return DefWindowProcA (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case CBEM_DELETEITEM:  /* maps to CB_DELETESTRING */
	    return COMBOEX_DeleteItem (hwnd, wParam, lParam);

	case CBEM_GETCOMBOCONTROL:
	    return COMBOEX_GetComboControl (hwnd, wParam, lParam);

	case CBEM_GETEDITCONTROL:
	    return COMBOEX_GetEditControl (hwnd, wParam, lParam);

	case CBEM_GETEXTENDEDSTYLE:
	    return COMBOEX_GetExtendedStyle (hwnd, wParam, lParam);

	case CBEM_GETIMAGELIST:
	    return COMBOEX_GetImageList (hwnd, wParam, lParam);

	case CBEM_GETITEMA:
	    return COMBOEX_GetItemA (hwnd, wParam, lParam);

	case CBEM_GETITEMW:
	    return COMBOEX_GetItemW (hwnd, wParam, lParam);

	case CBEM_GETUNICODEFORMAT:
	    return COMBOEX_GetUnicodeFormat (hwnd, wParam, lParam);

	case CBEM_HASEDITCHANGED:
	    return COMBOEX_HasEditChanged (hwnd, wParam, lParam);

	case CBEM_INSERTITEMA:
	    return COMBOEX_InsertItemA (hwnd, wParam, lParam);

	case CBEM_INSERTITEMW:
	    return COMBOEX_InsertItemW (hwnd, wParam, lParam);

	case CBEM_SETEXSTYLE:	/* FIXME: obsoleted, should be the same as: */
	case CBEM_SETEXTENDEDSTYLE:
	    return COMBOEX_SetExtendedStyle (hwnd, wParam, lParam);

	case CBEM_SETIMAGELIST:
	    return COMBOEX_SetImageList (hwnd, wParam, lParam);

	case CBEM_SETITEMA:
	    return COMBOEX_SetItemA (hwnd, wParam, lParam);

	case CBEM_SETITEMW:
	    return COMBOEX_SetItemW (hwnd, wParam, lParam);

	case CBEM_SETUNICODEFORMAT:
	    return COMBOEX_SetUnicodeFormat (hwnd, wParam, lParam);


/*   Combo messages we are not sure if we need to process or just forward */
	case CB_GETDROPPEDCONTROLRECT:
	case CB_GETDROPPEDSTATE:
	case CB_GETITEMDATA:
	case CB_GETITEMHEIGHT:
	case CB_GETLBTEXT:
	case CB_GETLBTEXTLEN:
	case CB_GETEXTENDEDUI:
	case CB_LIMITTEXT:
	case CB_RESETCONTENT:
	case CB_SELECTSTRING:
	case CB_SETITEMDATA:
	case WM_SETTEXT:
	case WM_GETTEXT:
	    FIXME("(0x%x 0x%x 0x%lx): possible missing fucntion\n", 
		  uMsg, wParam, lParam);
	    return COMBOEX_Forward (hwnd, uMsg, wParam, lParam);

/*   Combo messages OK to just forward to the regular COMBO */
	case CB_GETCOUNT:        
	case CB_GETCURSEL:
        case CB_SETDROPPEDWIDTH:
        case CB_SETEXTENDEDUI:
        case CB_SHOWDROPDOWN:
	    return COMBOEX_Forward (hwnd, uMsg, wParam, lParam);

/*   Combo messages we need to process specially */
        case CB_FINDSTRINGEXACT:
	    return COMBOEX_FindStringExact (COMBOEX_GetInfoPtr (hwnd), 
					    wParam, lParam);

	case CB_SETCURSEL:
	    return COMBOEX_SetCursel (hwnd, wParam, lParam);

	case CB_SETITEMHEIGHT:
	    return COMBOEX_SetItemHeight (hwnd, wParam, lParam);



/*   Window messages passed to parent */
	case WM_COMMAND:
	    return COMBOEX_Command (hwnd, wParam, lParam);

	case WM_NOTIFY:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);


/*   Window messages we need to process */
	case WM_CREATE:
	    return COMBOEX_Create (hwnd, wParam, lParam);

        case WM_DELETEITEM:
	    return COMBOEX_WM_DeleteItem (hwnd, wParam, lParam);

        case WM_DRAWITEM:
            return COMBOEX_DrawItem (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return COMBOEX_Destroy (hwnd, wParam, lParam);

        case WM_MEASUREITEM:
            return COMBOEX_MeasureItem (hwnd, wParam, lParam);

	case WM_SIZE:
	    return COMBOEX_Size (hwnd, wParam, lParam);

        case WM_WINDOWPOSCHANGING:
	    return COMBOEX_WindowPosChanging (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
COMBOEX_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)COMBOEX_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(COMBOEX_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_COMBOBOXEXA;
 
    RegisterClassA (&wndClass);

    ComboExInfo = GlobalAddAtomA("CC32SubclassInfo");
}


VOID
COMBOEX_Unregister (void)
{
    UnregisterClassA (WC_COMBOBOXEXA, (HINSTANCE)NULL);
}

