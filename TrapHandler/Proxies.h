
//
// Proxies.h : Declaration of the CProxies

#pragma once

#pragma warning (disable : 4996)

class CProxiesAccessor
{
public:
	LONG m_SNMPQueueLength;
	LONG m_CommandQueueLength;
	LONG m_HeartBeatTimeoutSecs;
	LONG m_MaxConcurrentCommands;
	LONG m_ConfigUpdateIntervalSecs;
	wchar_t m_LogMinSeverity[21];
	DBTIMESTAMP m_ThreshChangeTime;

	// The following wizard-generated data members contain status
	// values for the corresponding fields in the column map. You
	// can use these values to hold NULL values that the database
	// returns or to hold error information when the compiler returns
	// errors. See Field Status Data Members in Wizard-Generated
	// Accessors in the Visual C++ documentation for more information
	// on using these fields.
	// NOTE: You must initialize these fields before setting/inserting data!

	DBSTATUS m_dwSNMPQueueLengthStatus;
	DBSTATUS m_dwCommandQueueLengthStatus;
	DBSTATUS m_dwHeartBeatTimeoutSecsStatus;
	DBSTATUS m_dwMaxConcurrentCommandsStatus;
	DBSTATUS m_dwConfigUpdateIntervalSecsStatus;
	DBSTATUS m_dwLogMinSeverityStatus;
	DBSTATUS m_dwThreshChangeTimeStatus;

	// The following wizard-generated data members contain length
	// values for the corresponding fields in the column map.
	// NOTE: For variable-length columns, you must initialize these
	//       fields before setting/inserting data!

	DBLENGTH m_dwSNMPQueueLengthLength;
	DBLENGTH m_dwCommandQueueLengthLength;
	DBLENGTH m_dwHeartBeatTimeoutSecsLength;
	DBLENGTH m_dwMaxConcurrentCommandsLength;
	DBLENGTH m_dwConfigUpdateIntervalSecsLength;
	DBLENGTH m_dwLogMinSeverityLength;
	DBLENGTH m_dwThreshChangeTimeLength;


	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
	}

	operator const CSession&()
	{
		return *m_session;
	}

	CSession *m_session;
	wchar_t *dbCmd;

	/*
	DEFINE_COMMAND_EX(CProxiesAccessor, L" \
			SELECT \
				ProxyHostName, \
				SNMPQueueLength, \
				CommandQueueLength, \
				HeartBeatTimeoutSecs, \
				MaxConcurrentCommands, \
				ConfigUpdateIntervalSecs, \
				LogMinSeverity, \
				ThreshChangeTime \
				FROM dbo.Proxies")
				*/


	// In order to fix several issues with some providers, the code below may bind
	// columns in a different order than reported by the provider

	BEGIN_COLUMN_MAP(CProxiesAccessor)
		COLUMN_ENTRY_LENGTH_STATUS(1, m_SNMPQueueLength, m_dwSNMPQueueLengthLength, m_dwSNMPQueueLengthStatus)
		COLUMN_ENTRY_LENGTH_STATUS(2, m_CommandQueueLength, m_dwCommandQueueLengthLength, m_dwCommandQueueLengthStatus)
		COLUMN_ENTRY_LENGTH_STATUS(3, m_HeartBeatTimeoutSecs, m_dwHeartBeatTimeoutSecsLength, m_dwHeartBeatTimeoutSecsStatus)
		COLUMN_ENTRY_LENGTH_STATUS(4, m_MaxConcurrentCommands, m_dwMaxConcurrentCommandsLength, m_dwMaxConcurrentCommandsStatus)
		COLUMN_ENTRY_LENGTH_STATUS(5, m_ConfigUpdateIntervalSecs, m_dwConfigUpdateIntervalSecsLength, m_dwConfigUpdateIntervalSecsStatus)
		COLUMN_ENTRY_LENGTH_STATUS(6, m_LogMinSeverity, m_dwLogMinSeverityLength, m_dwLogMinSeverityStatus)
		COLUMN_ENTRY_LENGTH_STATUS(7, m_ThreshChangeTime, m_dwThreshChangeTimeLength, m_dwThreshChangeTimeStatus)
	END_COLUMN_MAP()
};

class CProxies : public CCommand<CAccessor<CProxiesAccessor> >
{
public:
	~CProxies() { if(dbCmd != NULL) delete [] dbCmd; }
	HRESULT OpenAll(CSession *session, const wchar_t *hostname)
	{
		m_session = session;

		dbCmd = new wchar_t [4996];
		wcscpy(dbCmd, L" \
			SELECT \
				SNMPQueueLength, \
				CommandQueueLength, \
				HeartBeatTimeoutSecs, \
				MaxConcurrentCommands, \
				ConfigUpdateIntervalSecs, \
				LogMinSeverity, \
				ThreshChangeTime \
				FROM dbo.Proxies WHERE ProxyHostName='");
		wcscat(dbCmd, hostname);
		wcscat(dbCmd, L"'");


		__if_exists(GetRowsetProperties)
		{
			CDBPropSet propset(DBPROPSET_ROWSET);
			__if_exists(HasBookmark)
			{
				if( HasBookmark() )
					propset.AddProperty(DBPROP_IRowsetLocate, true);
			}
			GetRowsetProperties(&propset);
			return OpenRowset(dbCmd, &propset);
		}

		__if_not_exists(GetRowsetProperties)
		{
			__if_exists(HasBookmark)
			{
				if( HasBookmark() )
				{
					CDBPropSet propset(DBPROPSET_ROWSET);
					propset.AddProperty(DBPROP_IRowsetLocate, true);
					return OpenRowset(dbCmd, &propset);
				}
			}
		}
		return OpenRowset(dbCmd);
	}

	HRESULT OpenRowset(const wchar_t *dbCmd, DBPROPSET *pPropSet = NULL)
	{
		HRESULT hr = Open(*m_session, dbCmd, pPropSet);
#ifdef _DEBUG
		if(FAILED(hr))
			AtlTraceErrorRecords(hr);
#endif
		return hr;
	}

	void CloseAll()
	{
		Close();
		ReleaseCommand();
	}
};


