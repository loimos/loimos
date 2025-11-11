/* Copyright 2020-2025 The Loimos Project Developers.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: MIT
 *
 * Stub definitions for Charm++ proxy classes used by InterventionModel.
 * This file provides minimal implementations of the proxy methods that
 * InterventionModel.cpp references, allowing unit tests to link.
 */

// Include the Charm++ declarations
#include "../loimos.decl.h"

// Define the global proxy instances (declared extern in Extern.h)
CProxy_People peopleArray;
CProxy_Locations locationsArray;
CProxy_Main mainProxy;
CProxy_Scenario globScenario;

// ============ CProxy_People method stubs ============

void CProxy_People::ReceiveIntervention(int interventionIdx, const CkEntryOptions *impl_e_opts) {
    // No-op stub for unit testing
}

void CProxy_People::SendVisitSchedules(const CkEntryOptions *impl_e_opts) {}
void CProxy_People::ReceiveExpectedVisitors(const ExpectedVisitorsMessage &msg, const CkEntryOptions *impl_e_opts) {}
void CProxy_People::SendVisitorStates(const CkEntryOptions *impl_e_opts) {}
void CProxy_People::SendVisitMessages(const CkEntryOptions *impl_e_opts) {}
void CProxy_People::ReceiveInteractions(const InteractionMessage &impl_noname_0, const CkEntryOptions *impl_e_opts) {}
void CProxy_People::EndOfDayStateUpdate(const CkEntryOptions *impl_e_opts) {}
void CProxy_People::SendStats(const CkEntryOptions *impl_e_opts) {}
void CProxy_People::AtSync(const CkEntryOptions *impl_e_opts) {}

// ============ CProxy_Locations method stubs ============

void CProxy_Locations::ReceiveIntervention(int interventionIdx, const CkEntryOptions *impl_e_opts) {
    // No-op stub for unit testing
}

void CProxy_Locations::ReceiveVisitSchedule(const VisitScheduleMessage &msg, const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::SendExpectedVisitors(const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::ReceiveVisitorStates(const PersonStatesMessage &msg, const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::QueueVisits(const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::ReceiveVisitMessages(const VisitMessage &impl_noname_1, const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::ComputeInteractions(const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::ReceiveVisitIntervention(const VisitInterventionMessage &msg, const CkEntryOptions *impl_e_opts) {}
void CProxy_Locations::AtSync(const CkEntryOptions *impl_e_opts) {}
