#ifndef __DBUTIL_INCLUDE
#define __DBUTIL_INCLUDE

#include <atldbcli.h>

// Derived class of CRowset, that implements the
// GetRecordCount and
// GetCurrentPosition, used to count the number of records returned from a query

// Execute a command with no rowset and accessor
HRESULT ExecuteDbCmd(CSession *dbSession, const wchar_t *sqlCmd)
{
    CCommand<CNoAccessor, CNoRowset> cmd;
    HRESULT hr = cmd.Open(*dbSession, sqlCmd);

    cmd.Close();
    return hr;
}

// execute a command returning dynamic accessor
CCommand<CDynamicAccessor, CRowset> *ExecuteDbSelectCmd(CSession *dbSession, const wchar_t *sqlCmd)
{
    CCommand<CDynamicAccessor, CRowset> *cmd = new CCommand<CDynamicAccessor, CRowset>();
    HRESULT hr = cmd->Open(*dbSession, sqlCmd);
    
    return cmd;

}


#endif