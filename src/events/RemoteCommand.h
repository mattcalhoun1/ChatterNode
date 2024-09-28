#ifndef REMOTECOMMAND_H
#define REMOTECOMMAND_H

#define REMOTE_COMMAND_REPORT_BATTERY "Report Battery"
#define REMOTE_COMMAND_REPORT_UPTIME "Report Uptime"
#define REMOTE_COMMAND_REPORT_NEIGHBORS "Report Neighbors"

#define REMOTE_COMMAND_PREFIX "CFG"

enum RemoteCommandType {
    RemoteCommandBattery = 'B',
    RemoteCommandPath = 'P',
    RemoteCommandMeshCacheClear = 'M',
    RemoteCommandMeshGraphClear = 'G',
    RemoteCommandPingTableClear = 'T',
    RemoteCommandEnableLearn = 'E',
    RemoteCommandDisableLearn = 'D',
    RemoteCommandMessagesClear = 'C',
    RemoteCommandUptime = 'U',
    RemoteCommandNeighbors = 'N',
    RemoteCommandUnknown = '?'
};

#endif