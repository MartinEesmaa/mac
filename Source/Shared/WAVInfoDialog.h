#ifndef APE_WAVINFODIALOG_H
#define APE_WAVINFODIALOG_H

BOOL CALLBACK FileInfoDialogProcedureA(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	
class CWAVInfoDialog
{
public:
	CWAVInfoDialog();
	~CWAVInfoDialog();

	long ShowWAVInfoDialog(const char *pFilename, HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent);

private:

	static LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	long InitDialog(HWND hDlg);
	char m_cFileName[MAX_PATH];
};

#endif // #ifndef APE_WAVINFODIALOG_H
