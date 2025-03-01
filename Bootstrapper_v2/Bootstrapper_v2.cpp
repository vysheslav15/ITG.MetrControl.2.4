﻿// Bootstrapper_v2.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Bootstrapper_v2.h"

#if !defined(PRODUCT_CODE) || !defined(CSMMAIN_CODE) || !defined(CSMADMIN_CODE) || !defined(MARKINV_CODE)
#error "PRODUCT_CODE, CSMMAIN_CODE, CSMADMIN_CODE or MARKINV_CODE variable expected! It must be set in project configuration."
#endif

template<>
struct std::less<LPCTSTR>
{	// functor for operator<
	bool operator()(const LPCTSTR _Left, const LPCTSTR _Right) const
	{
		return std::_tcscmp(_Left, _Right) < 0;
	}
};

const std::map<LPCTSTR, LPCTSTR> Tools({{
	{ _T("csmmain"), _T(CSMMAIN_CODE) },
	{ _T("csmadmin"), _T(CSMADMIN_CODE) },
	{ _T("markinv"), _T(MARKINV_CODE) }
}});

#define DEFAULT_STR_LENGTH 0x0400

int APIENTRY wWinMain
(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPTSTR lpCmdLine,
  _In_ int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	CTrace::SetLevel(4);
#endif
	
	HRESULT hr = ERROR_SUCCESS;
	CString appTitle = _T("Metrcontrol loader");

	try
	{

		ATLENSURE_SUCCEEDED(::CoInitialize(NULL));

		appTitle.LoadString(IDS_APP_TITLE);

#pragma region анализируем аргументы командной строки
		ATLENSURE_THROW((2 <= __argc) || (3 >= __argc), ERROR_BAD_ARGUMENTS);

		const LPCTSTR IniFilePath(__targv[1]);
		LPCTSTR ToolStrId(_T("csmmain"));
		if (3 == __argc)
		{
			ToolStrId = __targv[2];
			ATLENSURE_THROW(!(Tools.find(__targv[2]) == Tools.end()), ERROR_BAD_ARGUMENTS);
		}
		const LPCTSTR ToolCode = Tools.at(ToolStrId);
#pragma endregion

#pragma region Читаем ini файл дескриптора базы
		auto GetVarFromPrivateProfileString = [IniFilePath](LPCTSTR key)
		{
			CString value;
			::GetPrivateProfileString(_T("MetrControlDB"), key, NULL, value.GetBuffer(DEFAULT_STR_LENGTH), DEFAULT_STR_LENGTH, IniFilePath);
			value.ReleaseBuffer();
			return value;
		};

		CString Server = GetVarFromPrivateProfileString(_T("Server"));
		CString Database = GetVarFromPrivateProfileString(_T("Database"));
		CString Login = GetVarFromPrivateProfileString(_T("Login"));
		CString PasswordHash = GetVarFromPrivateProfileString(_T("PasswordHash"));
		CString NTLM = GetVarFromPrivateProfileString(_T("NTLM"));
#pragma endregion

#pragma region Запись файла конфигурации АИС Метрконтроль
		{ // для локализации и освобождения COM объектов до CoUninitialize
			CComHeapPtr<TCHAR> pszLocalAppDataPath;
			ATLENSURE_SUCCEEDED(::SHGetKnownFolderPath(
				FOLDERID_LocalAppData,
				CSIDL_PERSONAL | CSIDL_FLAG_CREATE,
				NULL,
				(PTSTR*)&pszLocalAppDataPath
			));
			CString ConfigFilePath(pszLocalAppDataPath);
			::PathAppend(ConfigFilePath.GetBuffer(MAX_PATH), _T("IFirst\\MetrControl\\CnnSettings.xml"));
			ConfigFilePath.ReleaseBuffer();
			ATLTRACE2(
				atlTraceGeneral, 4,
				_T("Путь файла конфигурации: \"%s\".\n"),
				ConfigFilePath.GetString()
			);

			MSXML2::IXMLDOMDocument2Ptr ConfigFileDoc(__uuidof(MSXML2::DOMDocument60));
			MSXML2::IXMLDOMNodePtr ConfigFileElement(ConfigFileDoc);
			auto EntriesNode = ConfigFileElement->appendChild((MSXML2::IXMLDOMNodePtr)(ConfigFileDoc->createElement(_T("entries"))));

			auto AddConfigEntry = [ConfigFileDoc, EntriesNode](LPCTSTR key, LPCTSTR value)
			{
				auto KeyNode = EntriesNode->appendChild((MSXML2::IXMLDOMNodePtr)(ConfigFileDoc->createElement(_T("entry"))));
				(KeyNode->attributes->setNamedItem((MSXML2::IXMLDOMNodePtr)(ConfigFileDoc->createAttribute(_T("key")))))->nodeTypedValue = key;
				KeyNode->nodeTypedValue = value;
			};

			AddConfigEntry(_T("Key"), _T("35AA16F410699668C19C86A111D7A1287FD6D0FB33490C9B"));
			AddConfigEntry(_T("IV"), _T("3CE48E956DFB399A"));
			AddConfigEntry(_T("iFirst.SP.Ryabkov.CrossSessionData.UniversalSettingsSaver key: Assembly version: "), _T("1.48.0.1"));
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: server"), Server.GetString());
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: db"), Database.GetString());
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: user"), Login.GetString());
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: passwd"), PasswordHash.GetString());
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: timeout"), _T("-1"));
			AddConfigEntry(_T("SQLConnectLib.SQLConnecter key: ntauth"), (NTLM == _T("yes")) ? _T("true") : _T("false"));

			ATLTRACE2(
				atlTraceGeneral, 4,
				_T("Подготовлено содержимое файла конфигурации:\n\n%s\n\n"),
				(LPCTSTR)(ConfigFileElement->xml)
			);

			IStreamPtr ConfigFileStream;
			ATLENSURE_SUCCEEDED(::SHCreateStreamOnFile(
				ConfigFilePath.GetString(),
				STGM_WRITE | STGM_CREATE,
				&ConfigFileStream
			));

			MSXML2::IMXWriterPtr ConfigFileWriter(__uuidof(MSXML2::MXXMLWriter60));
			ConfigFileWriter->encoding = _T("utf-8");
			ConfigFileWriter->byteOrderMark = VARIANT_TRUE;
			ConfigFileWriter->indent = VARIANT_TRUE;
			ConfigFileWriter->omitXMLDeclaration = VARIANT_FALSE;
			ConfigFileWriter->standalone = VARIANT_TRUE;
			ConfigFileWriter->output = &*ConfigFileStream;

			MSXML2::ISAXXMLReaderPtr xmlParser(__uuidof(MSXML2::SAXXMLReader60));
			ATLENSURE_SUCCEEDED(xmlParser->putContentHandler((MSXML2::ISAXContentHandlerPtr)ConfigFileWriter));
			ATLENSURE_SUCCEEDED(xmlParser->putDTDHandler((MSXML2::ISAXDTDHandlerPtr)ConfigFileWriter));
			ATLENSURE_SUCCEEDED(xmlParser->putErrorHandler((MSXML2::ISAXErrorHandlerPtr)ConfigFileWriter));
			ATLENSURE_SUCCEEDED(xmlParser->putProperty((unsigned short *)_T("http://xml.org/sax/properties/lexical-handler"), &*ConfigFileWriter));
			ATLENSURE_SUCCEEDED(xmlParser->putProperty((unsigned short *)_T("http://xml.org/sax/properties/declaration-handler"), &*ConfigFileWriter));

			ATLENSURE_SUCCEEDED(xmlParser->parse(&*ConfigFileDoc));
		};
#pragma endregion

		CString ToolFilePath;
#pragma region устанавливаем необходимые компоненты приложения и получаем путь к исполняемым файлам
		{
			LPCTSTR Product = _T(PRODUCT_CODE);
			DWORD ToolFilePathSize = MAX_PATH;
			{
				INSTALLSTATE ToolFeatureState = ::MsiQueryFeatureState(Product, ToolStrId);
				ATLENSURE_THROW(
					(INSTALLSTATE_LOCAL == ToolFeatureState) ||
					(INSTALLSTATE_SOURCE == ToolFeatureState) ||
					(INSTALLSTATE_ADVERTISED == ToolFeatureState),
					ERROR_INSTALL_NOTUSED
				);
			}
			{
				HRESULT hr = ::MsiProvideComponent(
					Product,
					ToolStrId,
					ToolCode,
					INSTALLMODE_DEFAULT,
					ToolFilePath.GetBuffer(MAX_PATH),
					&ToolFilePathSize
				);
				ATLENSURE_THROW(ERROR_SUCCESS == hr, hr);
			};
			ToolFilePath.ReleaseBuffer();
			ATLTRACE2(
				atlTraceGeneral, 4,
				_T("Необходимый компонент установлен. Путь к исполняемому файлу затребованного компонента (%s): \n\"%s\".\n"),
				ToolStrId,
				ToolFilePath.GetString()
			);
		};
#pragma endregion

#pragma region запускаем приложение
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			if (!::CreateProcess(
				ToolFilePath.GetString(),
				NULL,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi
			))
				AtlThrowLastWin32();
			ATLTRACE2(
				atlTraceGeneral, 4,
				_T("Запущено приложение. Путь к исполняемому файлу затребованного компонента (%s): \n\"%s\".\n"),
				ToolStrId,
				ToolFilePath.GetString()
			);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		};
#pragma endregion

	}
	catch (HRESULT e)
	{
		hr = e;
	}
	catch (ATL::CAtlException e)
	{
		hr = e.m_hr;
	}
	catch (...)
	{
		hr = ERROR_UNIDENTIFIED_ERROR;
	}

#pragma region выводим сообщение об ошибке пользователю
	if (ERROR_SUCCESS != hr)
	{
		_com_error cerr(hr);
		CString ErrorMessage;
		ErrorMessage.Format(
			IDS_ERROR_MESSAGE_TEMPLATE,
			hr,
			cerr.ErrorMessage()
		);
		ATLTRACE2(
			atlTraceGeneral, 0,
			ErrorMessage
		);
		::MessageBox(0, ErrorMessage.GetString(), appTitle.GetString(), MB_ICONERROR | MB_OK);
	};
#pragma endregion

	::CoUninitialize();

    return hr;
}
