#pragma once

class CFSThresholdsAccessor
{
public:
	LONG m_TreshFreeMBWarn;
	LONG m_ThreshFreeMBHigh;
	double m_ThreshUtilWarn;
	double m_ThreshUtilHigh;
	TCHAR m_WarnSev[21];
	TCHAR m_highSev[21];
	TCHAR m_HostExpression[8000];
	TCHAR m_VolumeExpression[8000];
	LONG m_MinFSSizeMB;
	LONG m_MaxFSSizeMB;

	DBSTATUS m_dwTreshFreeMBWarnStatus;
	DBSTATUS m_dwThreshFreeMBHighStatus;
	DBSTATUS m_dwThreshUtilWarnStatus;
	DBSTATUS m_dwThreshUtilHighStatus;
	DBSTATUS m_dwWarnSevStatus;
	DBSTATUS m_dwhighSevStatus;
	DBSTATUS m_dwHostExpressionStatus;
	DBSTATUS m_dwVolumeExpressionStatus;
	DBSTATUS m_dwMinFSSizeMBStatus;
	DBSTATUS m_dwMaxFSSizeMBStatus;

	DBLENGTH m_dwTreshFreeMBWarnLength;
	DBLENGTH m_dwThreshFreeMBHighLength;
	DBLENGTH m_dwThreshUtilWarnLength;
	DBLENGTH m_dwThreshUtilHighLength;
	DBLENGTH m_dwWarnSevLength;
	DBLENGTH m_dwhighSevLength;
	DBLENGTH m_dwHostExpressionLength;
	DBLENGTH m_dwVolumeExpressionLength;
	DBLENGTH m_dwMinFSSizeMBLength;
	DBLENGTH m_dwMaxFSSizeMBLength;

	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, false, DBPROPOPTIONS_OPTIONAL);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, false, DBPROPOPTIONS_OPTIONAL);
		// pPropSet->AddProperty(DBPROP_ISequentialStream, true);
	}

	operator const CSession&()
	{
		return *m_session;
	}

	CSession *m_session;

	DEFINE_COMMAND_EX(CFSThresholdsAccessor, L" \
	SELECT \
		TreshFreeMBWarn, \
		ThreshFreeMBHigh, \
		ThreshUtilWarn, \
		ThreshUtilHigh, \
		WarnSev, \
		highSev, \
		HostExpression, \
		VolumeExpression, \
		MinFSSizeMB, \
		MaxFSSizeMB \
		FROM dbo.FSThresholds ORDER BY EvalOrder")


	// In order to fix several issues with some providers, the code below may bind
	// columns in a different order than reported by the provider

	BEGIN_COLUMN_MAP(CFSThresholdsAccessor)
		COLUMN_ENTRY_LENGTH_STATUS(1, m_TreshFreeMBWarn, m_dwTreshFreeMBWarnLength, m_dwTreshFreeMBWarnStatus)
		COLUMN_ENTRY_LENGTH_STATUS(2, m_ThreshFreeMBHigh, m_dwThreshFreeMBHighLength, m_dwThreshFreeMBHighStatus)
		COLUMN_ENTRY_LENGTH_STATUS(3, m_ThreshUtilWarn, m_dwThreshUtilWarnLength, m_dwThreshUtilWarnStatus)
		COLUMN_ENTRY_LENGTH_STATUS(4, m_ThreshUtilHigh, m_dwThreshUtilHighLength, m_dwThreshUtilHighStatus)
		COLUMN_ENTRY_LENGTH_STATUS(5, m_WarnSev, m_dwWarnSevLength, m_dwWarnSevStatus)
		COLUMN_ENTRY_LENGTH_STATUS(6, m_highSev, m_dwhighSevLength, m_dwhighSevStatus)
		COLUMN_ENTRY_LENGTH_STATUS(7, m_HostExpression, m_dwHostExpressionLength, m_dwHostExpressionStatus)
		COLUMN_ENTRY_LENGTH_STATUS(8, m_VolumeExpression, m_dwVolumeExpressionLength, m_dwVolumeExpressionStatus)
		COLUMN_ENTRY_LENGTH_STATUS(9,  m_MinFSSizeMB, m_dwMinFSSizeMBLength, m_dwMinFSSizeMBStatus)
		COLUMN_ENTRY_LENGTH_STATUS(10, m_MaxFSSizeMB, m_dwMaxFSSizeMBLength, m_dwMaxFSSizeMBStatus)
	END_COLUMN_MAP()
};

class CFSThresholds : public CCommand<CAccessor<CFSThresholdsAccessor> >
{
public:
	HRESULT OpenAll(CSession *session)
	{
		m_session = session;
		__if_exists(GetRowsetProperties)
		{
			CDBPropSet propset(DBPROPSET_ROWSET);
			__if_exists(HasBookmark)
			{
				if( HasBookmark() )
					propset.AddProperty(DBPROP_IRowsetLocate, true);
			}
			GetRowsetProperties(&propset);
			return OpenRowset(&propset);
		}
		__if_not_exists(GetRowsetProperties)
		{
			__if_exists(HasBookmark)
			{
				if( HasBookmark() )
				{
					CDBPropSet propset(DBPROPSET_ROWSET);
					propset.AddProperty(DBPROP_IRowsetLocate, true);
					return OpenRowset(&propset);
				}
			}
		}
		return OpenRowset();
	}

	HRESULT OpenRowset(DBPROPSET *pPropSet = NULL)
	{
		HRESULT hr = Open(*m_session, NULL, pPropSet);
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


