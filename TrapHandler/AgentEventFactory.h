#pragma once

#include "CSnmpTrap.h"
#include "ClusterFailEvent.h"
#include "DiskMetricEvent.h"
#include "ErrorEvent.h"
#include "GoodbyeEvent.h"
#include "HelloEvent.h"
#include "ProcessFailEvent.h"
#include "ServiceFailEvent.h"
#include "StartMaintenanceEvent.h"
#include "GenericEvent.h"

namespace traphandler
{
	namespace events
	{


// Factory that spawns the correct agent event, given an incomming snmp
// trap
class AgentEventFactory
{
public:
	traphandler::events::AgentEvent *GetEventFromTrap(CSnmpTrap &source_trap)
	{
		traphandler::events::AgentEvent *newEvent = nullptr;
		if (source_trap.TrapOid == &TrapOidDiskMetric)
			newEvent = new traphandler::events::DiskMetricEvent();
		else if (source_trap.TrapOid == &TrapOidProcFail)
			newEvent = new traphandler::events::ProcessFailEvent();
		else if (source_trap.TrapOid == &TrapOidServFail)
			newEvent = new traphandler::events::ServiceFailEvent();
		else if (source_trap.TrapOid == &TrapOidClusFail)
			newEvent = new traphandler::events::ClusterFailEvent();
		else if (source_trap.TrapOid == &TrapOidStartMaint)
			newEvent = new traphandler::events::StartMaintenanceEvent();
		else if (source_trap.TrapOid == &TrapOidError)
			newEvent = new traphandler::events::ErrorEvent();
		else if (source_trap.TrapOid == &TrapOidGoodbye)
			newEvent = new traphandler::events::GoodbyeEvent();
		else
			newEvent = new traphandler::events::GenericEvent();
		
		return newEvent;
	}
	AgentEventFactory() :
		TrapOidDiskMetric(L".1.3.6.1.4.1.15282.10.0.2"),
		TrapOidProcFail(L".1.3.6.1.4.1.15282.10.0.3"),
		TrapOidServFail(L".1.3.6.1.4.1.15282.10.0.4"),
		TrapOidClusFail(L".1.3.6.1.4.1.15282.10.0.5"),
		TrapOidStartMaint(L".1.3.6.1.4.1.15282.10.0.6"),
		TrapOidError(L".1.3.6.1.4.1.15282.10.0.7"),
		TrapOidHello(L".1.3.6.1.4.1.15282.10.0.8"),
		TrapOidGoodbye(L".1.3.6.1.4.1.15282.10.0.9")
	{

	}
private:
#pragma region TrapSpecifications
	// gcom defined traps
	// 1 = local hostname
	// 2 = volumeID
	// 3 = Total space MB
	// 4 = Free space MB
	CSnmpOid TrapOidDiskMetric;
	// 1 = local hostname
	// 2 = process name
	// 3 = current state
	// 4 = normal state
	CSnmpOid TrapOidProcFail;
	// 1 = local hostname
	// 2 = service name
	// 3 = current state
	// 4 = desired state
	// 5 = label
	CSnmpOid TrapOidServFail;
	// 1 = local hostname
	// 2 = Resource group name
	// 3 = Cluster name
	// 4 = Current state
	CSnmpOid TrapOidClusFail;
	// 1 = local hostname
	// 2 = user name
	// 3 = description
	// 4 = duration in minutes
	CSnmpOid TrapOidStartMaint;
	// 1 = local hostname
	// 2 = message
	// 3 = severity
	CSnmpOid TrapOidError;
	// 1 = local hostname
	CSnmpOid TrapOidHello;
	// 1 = local hostname
	CSnmpOid TrapOidGoodbye;
#pragma endregion
};

	}
}