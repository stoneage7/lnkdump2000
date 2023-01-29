
/*****
 * Part of LnkDump2000
 * Licence: GPL, version 3 or later (see COPYING file or https://www.gnu.org/licenses/gpl-3.0.txt)
 *****/

#include <cstring>
#include <ctime>
#include "encoding.h"
#include "struct.h"

namespace LnkStruct {

/*
taken from:
https://github.com/libyal/libfwsi/blob/main/documentation/Windows%20Shell%20Item%20format.asciidoc
*/

struct ShellFolderGuid {
    const char* guid;
    const char* description;
};

constexpr ShellFolderGuid shell_folder_guids[] = {
    {"00020D75-0000-0000-C000-000000000046", "Inbox"},
    {"00020D76-0000-0000-C000-000000000046", "Inbox"},
    {"00C6D95F-329C-409A-81D7-C46C66EA7F33", "Default Location"},
    {"0142E4D0-FB7A-11DC-BA4A-000FFE7AB428", "Biometric Devices (Biometrics)"},
    {"025A5937-A6BE-4686-A844-36FE4BEC8B6D", "Power Options"},
    {"031E4825-7B94-4DC3-B131-E946B44C8DD5", "Users Libraries"},
    {"04731B67-D933-450A-90E6-4ACD2E9408FE", "Search Folder"},
    {"05D7B0F4-2121-4EFF-BF6B-ED3F69B894D9", "Taskbar (Notification Area Icons)"},
    {"0875DCB6-C686-4243-9432-ADCCF0B9F2D7", "Microsoft !OneNote Namespace Extension for Windows Desktop Search"},
    {"0AFACED1-E828-11D1-9187-B532F1E9575D", "Folder Shortcut"},
    {"0BD8E793-D371-11D1-B0B5-0060972919D7", "!SolidWorks Enterprise PDM"},
    {"0CD7A5C0-9F37-11CE-AE65-08002B2E1262", "Cabinet File"},
    {"0DF44EAA-FF21-4412-828E-260A8728E7F1", "Taskbar and Start Menu"},
    {"11016101-E366-4D22-BC06-4ADA335C892B", "Internet Explorer History and Feeds Shell Data Source for Windows Search"},
    {"1206F5F1-0569-412C-8FEC-3204630DFB70", "Credential Manager"},
    {"13E7F612-F261-4391-BEA2-39DF4F3FA311", "Windows Desktop Search"},
    {"15EAE92E-F17A-4431-9F28-805E482DAFD4", "Install New Programs (Get Programs)"},
    {"1723D66A-7A12-443E-88C7-05E1BFE79983", "Previous Versions Delegate Folder"},
    {"17CD9488-1228-4B2F-88CE-4298E93E0966", "Default Programs (Set User Defaults)"},
    {"1A9BA3A0-143A-11CF-8350-444553540000", "Shell Favorite Folder"},
    {"1D2680C9-0E2A-469D-B787-065558BC7D43", "Fusion Cache"},
    {"1F3427C8-5C10-4210-AA03-2EE45287D668", "User Pinned"},
    {"1F43A58C-EA28-43E6-9EC4-34574A16EBB7", "Windows Desktop Search MAPI Namespace Extension Class"},
    {"1F4DE370-D627-11D1-BA4F-00A0C91EEDBA", "Search Results - Computers (Computer Search Results Folder, Network Computers)"},
    {"1FA9085F-25A2-489B-85D4-86326EEDCD87", "Manage Wireless Networks"},
    {"208D2C60-3AEA-1069-A2D7-08002B30309D", "My Network Places (Network)"},
    {"20D04FE0-3AEA-1069-A2D8-08002B30309D", "My Computer (Computer)"},
    {"21EC2020-3AEA-1069-A2DD-08002B30309D", "Control Panel"},
    {"2227A280-3AEA-1069-A2DE-08002B30309D", "Printers and Faxes (Printers)"},
    {"241D7C96-F8BF-4F85-B01F-E2B043341A4B", "Workspaces Center (Remote Application and Desktop Connections)"},
    {"2559A1F0-21D7-11D4-BDAF-00C04F60B9F0", "Search"},
    {"2559A1F1-21D7-11D4-BDAF-00C04F60B9F0", "Help and Support"},
    {"2559A1F2-21D7-11D4-BDAF-00C04F60B9F0", "Windows Security"},
    {"2559A1F3-21D7-11D4-BDAF-00C04F60B9F0", "Run..."},
    {"2559A1F4-21D7-11D4-BDAF-00C04F60B9F0", "Internet"},
    {"2559A1F5-21D7-11D4-BDAF-00C04F60B9F0", "E-mail"},
    {"2559A1F7-21D7-11D4-BDAF-00C04F60B9F0", "Set Program Access and Defaults"},
    {"267CF8A9-F4E3-41E6-95B1-AF881BE130FF", "Location Folder"},
    {"26EE0668-A00A-44D7-9371-BEB064C98683", "Control Panel"},
    {"2728520D-1EC8-4C68-A551-316B684C4EA7", "Network Setup Wizard"},
    {"28803F59-3A75-4058-995F-4EE5503B023C", "Bluetooth Devices"},
    {"289978AC-A101-4341-A817-21EBA7FD046D", "Sync Center Conflict Folder"},
    {"289AF617-1CC3-42A6-926C-E6A863F0E3BA", "DLNA Media Servers Data Source"},
    {"2965E715-EB66-4719-B53F-1672673BBEFA", "Results Folder"},
    {"2E9E59C0-B437-4981-A647-9C34B9B90891", "Sync Setup Folder"},
    {"2F6CE85C-F9EE-43CA-90C7-8A9BD53A2467", "File History Data Source"},
    {"3080F90D-D7AD-11D9-BD98-0000947B0257", "Show Desktop"},
    {"3080F90E-D7AD-11D9-BD98-0000947B0257", "Window Switcher"},
    {"323CA680-C24D-4099-B94D-446DD2D7249E", "Common Places"},
    {"328B0346-7EAF-4BBE-A479-7CB88A095F5B", "Layout Folder"},
    {"335A31DD-F04B-4D76-A925-D6B47CF360DF", "Backup and Restore Center"},
    {"35786D3C-B075-49B9-88DD-029876E11C01", "Portable Devices"},
    {"36EEF7DB-88AD-4E81-AD49-0E313F0C35F8", "Windows Update"},
    {"3C5C43A3-9CE9-4A9B-9699-2AC0CF6CC4BF", "Configure Wireless Network"},
    {"3F6BC534-DFA1-4AB4-AE54-EF25A74E0107", "System Restore"},
    {"4026492F-2F69-46B8-B9BF-5654FC07E423", "Windows Firewall"},
    {"418C8B64-5463-461D-88E0-75E2AFA3C6FA", "Explorer Browser Results Folder"},
    {"4234D49B-0245-4DF3-B780-3893943456E1", "Applications"},
    {"437FF9C0-A07F-4FA0-AF80-84B6C6440A16", "Command Folder"},
    {"450D8FBA-AD25-11D0-98A8-0800361B1103", "My Documents"},
    {"48E7CAAB-B918-4E58-A94D-505519C795DC", "Start Menu Folder"},
    {"5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0", "Control Panel command object for Start menu and desktop"},
    {"58E3C745-D971-4081-9034-86E34B30836A", "Speech Recognition Options"},
    {"59031A47-3F72-44A7-89C5-5595FE6B30EE", "Shared Documents Folder (Users Files)"},
    {"5EA4F148-308C-46D7-98A9-49041B1DD468", "Mobility Center Control Panel"},
    {"60632754-C523-4B62-B45C-4172DA012619", "User Accounts"},
    {"63DA6EC0-2E98-11CF-8D82-444553540000", "Microsoft FTP Folder"},
    {"640167B4-59B0-47A6-B335-A6B3C0695AEA", "Portable Media Devices"},
    {"645FF040-5081-101B-9F08-00AA002F954E", "Recycle Bin"},
    {"67718415-C450-4F3C-BF8A-B487642DC39B", "Windows Features"},
    {"6785BFAC-9D2D-4BE5-B7E2-59937E8FB80A", "Other Users Folder"},
    {"67CA7650-96E6-4FDD-BB43-A8E774F73A57", "Home Group Control Panel (Home Group)"},
    {"692F0339-CBAA-47E6-B5B5-3B84DB604E87", "Extensions Manager Folder"},
    {"6DFD7C5C-2451-11D3-A299-00C04F8EF6AF", "Folder Options"},
    {"7007ACC7-3202-11D1-AAD2-00805FC1270E", "Network Connections (Network and Dial-up Connections)"},
    {"708E1662-B832-42A8-BBE1-0A77121E3908", "Tree property value folder"},
    {"71D99464-3B6B-475C-B241-E15883207529", "Sync Results Folder"},
    {"72B36E70-8700-42D6-A7F7-C9AB3323EE51", "Search Connector Folder"},
    {"78F3955E-3B90-4184-BD14-5397C15F1EFC", "Performance Information and Tools"},
    {"7A9D77BD-5403-11D2-8785-2E0420524153", "User Accounts (Users and Passwords)"},
    {"7B81BE6A-CE2B-4676-A29E-EB907A5126C5", "Programs and Features"},
    {"7BD29E00-76C1-11CF-9DD0-00A0C9034933", "Temporary Internet Files"},
    {"7BD29E01-76C1-11CF-9DD0-00A0C9034933", "Temporary Internet Files"},
    {"7BE9D83C-A729-4D97-B5A7-1B7313C39E0A", "Programs Folder"},
    {"8060B2E3-C9D7-4A5D-8C6B-CE8EBA111328", "Proximity CPL"},
    {"8343457C-8703-410F-BA8B-8B026E431743", "Feedback Tool"},
    {"85BBD920-42A0-1069-A2E4-08002B30309D", "Briefcase"},
    {"863AA9FD-42DF-457B-8E4D-0DE1B8015C60", "Remote Printers"},
    {"865E5E76-AD83-4DCA-A109-50DC2113CE9A", "Programs Folder and Fast Items"},
    {"871C5380-42A0-1069-A2EA-08002B30309D", "Internet Explorer (Homepage)"},
    {"87630419-6216-4FF8-A1F0-143562D16D5C", "Mobile Broadband Profile Settings Editor"},
    {"877CA5AC-CB41-4842-9C69-9136E42D47E2", "File Backup Index"},
    {"88C6C381-2E85-11D0-94DE-444553540000", "ActiveX Cache Folder"},
    {"896664F7-12E1-490F-8782-C0835AFD98FC", "Libraries delegate folder that appears in Users Files Folder"},
    {"8E908FC9-BECC-40F6-915B-F4CA0E70D03D", "Network and Sharing Center"},
    {"8FD8B88D-30E1-4F25-AC2B-553D3D65F0EA", "DXP"},
    {"9113A02D-00A3-46B9-BC5F-9C04DADDD5D7", "Enhanced Storage Data Source"},
    {"93412589-74D4-4E4E-AD0E-E0CB621440FD", "Font Settings"},
    {"9343812E-1C37-4A49-A12E-4B2D810D956B", "Search Home"},
    {"96437431-5A90-4658-A77C-25478734F03E", "Server Manager"},
    {"96AE8D84-A250-4520-95A5-A47A7E3C548B", "Parental Controls"},
    {"98D99750-0B8A-4C59-9151-589053683D73", "Windows Search Service Media Center Namespace Extension Handler"},
    {"992CFFA0-F557-101A-88EC-00DD010CCC48", "Network Connections (Network and Dial-up Connections)"},
    {"9A096BB5-9DC3-4D1C-8526-C3CBF991EA4E", "Internet Explorer RSS Feeds Folder"},
    {"9C60DE1E-E5FC-40F4-A487-460851A8D915", "AutoPlay"},
    {"9C73F5E5-7AE7-4E32-A8E8-8D23B85255BF", "Sync Center Folder"},
    {"9DB7A13C-F208-4981-8353-73CC61AE2783", "Previous Versions"},
    {"9F433B7C-5F96-4CE1-AC28-AEAA1CC04D7C", "Security Center"},
    {"9FE63AFD-59CF-4419-9775-ABCC3849F861", "System Recovery (Recovery)"},
    {"A3C3D402-E56C-4033-95F7-4885E80B0111", "Previous Versions Results Delegate Folder"},
    {"A5A3563A-5755-4A6F-854E-AFA3230B199F", "Library Folder"},
    {"A5E46E3A-8849-11D1-9D8C-00C04FC99D61", "Microsoft Browser Architecture"},
    {"A6482830-08EB-41E2-84C1-73920C2BADB9", "Removable Storage Devices"},
    {"A8A91A66-3A7D-4424-8D24-04E180695C7A", "Device Center (Devices and Printers)"},
    {"AEE2420F-D50E-405C-8784-363C582BF45A", "Device Pairing Folder"},
    {"AFDB1F70-2A4C-11D2-9039-00C04F8EEB3E", "Offline Files Folder"},
    {"B155BDF8-02F0-451E-9A26-AE317CFD7779", "Nethood delegate folder (Delegate folder that appears in Computer)"},
    {"B2952B16-0E07-4E5A-B993-58C52CB94CAE", "DB Folder"},
    {"B4FB3F98-C1EA-428D-A78A-D1F5659CBA93", "Other Users Folder"},
    {"B98A2BEA-7D42-4558-8BD1-832F41BAC6FD", "Backup And Restore (Backup and Restore Center)"},
    {"BB06C0E4-D293-4F75-8A90-CB05B6477EEE", "System"},
    {"BB64F8A7-BEE7-4E1A-AB8D-7D8273F7FDB6", "Action Center Control Panel"},
    {"BC476F4C-D9D7-4100-8D4E-E043F6DEC409", "Microsoft Browser Architecture"},
    {"BC48B32F-5910-47F5-8570-5074A8A5636A", "Sync Results Delegate Folder"},
    {"BD84B380-8CA2-1069-AB1D-08000948F534", "Microsoft Windows Font Folder"},
    {"BDEADF00-C265-11D0-BCED-00A0C90AB50F", "Web Folders"},
    {"BE122A0E-4503-11DA-8BDE-F66BAD1E3F3A", "Windows Anytime Upgrade"},
    {"BF782CC9-5A52-4A17-806C-2A894FFEEAC5", "Language Settings"},
    {"C291A080-B400-4E34-AE3F-3D2B9637D56C", "UNCFATShellFolder Class"},
    {"C2B136E2-D50E-405C-8784-363C582BF43E", "Device Center Initialization"},
    {"C555438B-3C23-4769-A71F-B6D3D9B6053A", "Display"},
    {"C57A6066-66A3-4D91-9EB9-41532179F0A5", "Application Suggested Locations"},
    {"C58C4893-3BE0-4B45-ABB5-A63E4B8C8651", "Troubleshooting"},
    {"CB1B7F8C-C50A-4176-B604-9E24DEE8D4D1", "Welcome Center (Getting Started)"},
    {"D2035EDF-75CB-4EF1-95A7-410D9EE17170", "DLNA Content Directory Data Source"},
    {"D20EA4E1-3957-11D2-A40B-0C5020524152", "Fonts"},
    {"D20EA4E1-3957-11D2-A40B-0C5020524153", "Administrative Tools"},
    {"D34A6CA6-62C2-4C34-8A7C-14709C1AD938", "Common Places FS Folder"},
    {"D426CFD0-87FC-4906-98D9-A23F5D515D61", "Windows Search Service Outlook Express Protocol Handler"},
    {"D4480A50-BA28-11D1-8E75-00C04FA31A86", "Add Network Place"},
    {"D450A8A1-9568-45C7-9C0E-B4F9FB4537BD", "Installed Updates"},
    {"D555645E-D4F8-4C29-A827-D93C859C4F2A", "Ease of Access (Ease of Access Center)"},
    {"D5B1944E-DB4E-482E-B3F1-DB05827F0978", "Softex OmniPass Encrypted Folder"},
    {"D6277990-4C6A-11CF-8D87-00AA0060F5BF", "Scheduled Tasks"},
    {"D8559EB9-20C0-410E-BEDA-7ED416AECC2A", "Windows Defender"},
    {"D9EF8727-CAC2-4E60-809E-86F80A666C91", "Secure Startup (BitLocker Drive Encryption)"},
    {"DFFACDC5-679F-4156-8947-C5C76BC0B67F", "Delegate folder that appears in Users Files Folder"},
    {"E17D4FC0-5564-11D1-83F2-00A0C90DC849", "Search Results Folder"},
    {"E211B736-43FD-11D1-9EFB-0000F8757FCD", "Scanners and Cameras"},
    {"E413D040-6788-4C22-957E-175D1C513A34", "Sync Center Conflict Delegate Folder"},
    {"E773F1AF-3A65-4866-857D-846FC9C4598A", "Shell Storage Folder Viewer"},
    {"E7DE9B1A-7533-4556-9484-B26FB486475E", "Network Map"},
    {"E7E4BC40-E76A-11CE-A9BB-00AA004AE837", "Shell DocObject Viewer"},
    {"E88DCCE0-B7B3-11D1-A9F0-00AA0060FA31", "Compressed Folder"},
    {"E95A4861-D57A-4BE1-AD0F-35267E261739", "Windows SideShow"},
    {"E9950154-C418-419E-A90A-20C5287AE24B", "Sensors (Location and Other Sensors)"},
    {"ED228FDF-9EA8-4870-83B1-96B02CFE0D52", "My Games (Games Explorer)"},
    {"ED50FC29-B964-48A9-AFB3-15EBB9B97F36", "PrintHood delegate folder"},
    {"ED7BA470-8E54-465E-825C-99712043E01C", "All Tasks"},
    {"ED834ED6-4B5A-4BFE-8F11-A626DCB6A921", "Personalization Control Panel"},
    {"EDC978D6-4D53-4B2F-A265-5805674BE568", "Stream Backed Folder"},
    {"F02C1A0D-BE21-4350-88B0-7367FC96EF3C", "Computers and Devices"},
    {"F1390A9A-A3F4-4E5D-9C5F-98F3BD8D935C", "Sync Setup Delegate Folder"},
    {"F3F5824C-AD58-4728-AF59-A1EBE3392799", "Sticky Notes Namespace Extension for Windows Desktop Search"},
    {"F5175861-2688-11D0-9C5E-00AA00A45957", "Subscription Folder"},
    {"F6B6E965-E9B2-444B-9286-10C9152EDBC5", "History Vault"},
    {"F8C2AB3B-17BC-41DA-9758-339D7DBF2D88", "Previous Versions Results Folder"},
    {"F90C627B-7280-45DB-BC26-CCE7BDD620A4", "All Tasks"},
    {"F942C606-0914-47AB-BE56-1321B8035096", "Storage Spaces"},
    {"FB0C9C8A-6C50-11D1-9F1D-0000F8757FCD", "Scanners & Cameras"},
    {"FBF23B42-E3F0-101B-8488-00AA003E56F8", "Internet Explorer"},
    {"FE1290F0-CFBD-11CF-A330-00AA00C16E65", "Directory"},
    {"FF393560-C2A7-11CF-BFF4-444553540000", "History"},
    {"9D20AAE8-0625-44B0-9CA7-71889C2254D9", "UNIX Folder"},  // seems like WINE stuff
    {nullptr, nullptr}
};

const char* shell_folder_guid_describe(const char* guid)
{
    const auto& g = shell_folder_guids;
    const auto* i = &g[0];
    if (guid != nullptr) {
        while (i->guid != nullptr) {
            if (strcmp(i->guid, guid) == 0) {
                return i->description;
            }
            i++;
        }
    }
    return nullptr;
}

struct ControlPanelGuid {
    const char* guid;
    const char* description;
};

static const ControlPanelGuid control_panel_guids[] = {
    {"00F2886F-CD64-4FC9-8EC5-30EF6CDBE8C3", "Scanners and Cameras"},
    {"087DA31B-0DD3-4537-8E23-64A18591F88B", "Windows Security Center"},
    {"259EF4B1-E6C9-4176-B574-481532C9BCE8", "Game Controllers"},
    {"36EEF7DB-88AD-4E81-AD49-0E313F0C35F8", "Windows Update"},
    {"37EFD44D-EF8D-41B1-940D-96973A50E9E0", "Windows Sidebar Properties (Desktop Gadgets)"},
    {"3E7EFB4C-FAF1-453D-89EB-56026875EF90", "Windows Marketplace"},
    {"40419485-C444-4567-851A-2DD7BFA1684D", "Phone and Modem"},
    {"5224F545-A443-4859-BA23-7B5A95BDC8EF", "People Near Me"},
    {"62D8ED13-C9D0-4CE8-A914-47DD628FB1B0", "Regional and Language Options"},
    {"6C8EEC18-8D75-41B2-A177-8831D59D2D50", "Mouse"},
    {"7007ACC7-3202-11D1-AAD2-00805FC1270E", "Connections"},
    {"725BE8F7-668E-4C7B-8F90-46BDB0936430", "Keyboard"},
    {"74246BFC-4C96-11D0-ABEF-0020AF6B0B7A", "Device Manager"},
    {"78CB147A-98EA-4AA6-B0DF-C8681F69341C", "Windows CardSpace"},
    {"7A979262-40CE-46FF-AEEE-7884AC3B6136", "Add Hardware"},
    {"80F3F1D5-FECA-45F3-BC32-752C152E456E", "Tablet PC Settings"},
    {"87D66A43-7B11-4A28-9811-C86EE395ACF7", "Indexing Options"},
    {"8E908FC9-BECC-40F6-915B-F4CA0E70D03D", "Network and Sharing Center"},
    {"A0275511-0E86-4ECA-97C2-ECD8F1221D08", "Infrared"},
    {"A304259D-52B8-4526-8B1A-A1D6CECC8243", "iSCSI Initiator"},
    {"A3DD4F92-658A-410F-84FD-6FBBBEF2FFFE", "Internet Options"},
    {"B2C761C6-29BC-4F19-9251-E6195265BAF1", "Color Management"},
    {"BB06C0E4-D293-4F75-8A90-CB05B6477EEE", "System"},
    {"BB64F8A7-BEE7-4E1A-AB8D-7D8273F7FDB6", "Action Center"},
    {"D17D1D6D-CC3F-4815-8FE3-607E7D5D10B3", "Text to Speech"},
    {"D24F75AA-4F2B-4D07-A3C4-469B3D9030C4", "Offline Files"},
    {"E2E7934B-DCE5-43C4-9576-7FE4F75E7480", "Date and Time"},
    {"F2DDFC82-8F12-4CDD-B7DC-D4FE1425AA4D", "Sound"},
    {"F82DF8F7-8B9F-442E-A48C-818EA735FF9B", "Pen and Input Devices (Pen and Touch)"},
    {"FCFEECAE-EE1B-4849-AE50-685DCF7717EC", "Problem Reports and Solutions"},
    {nullptr, nullptr}
};

const char* control_panel_guid_describe(const char* guid)
{
    const auto& g = control_panel_guids;
    const auto* i = &g[0];
    if (guid != nullptr) {
        while (i->guid != nullptr) {
            if (strcmp(i->guid, guid) == 0) {
                return i->description;
            }
            i++;
        }
    }
    return nullptr;
}

time_t
FATTime::unix_time()
{
    struct tm u;
    uint16_t lo_word = get_bits< 0, 15>(m_fat);
    uint16_t hi_word = get_bits<16, 31>(m_fat);
    u.tm_sec         = get_bits< 0,  4>(hi_word) * 2;
    u.tm_min         = get_bits< 5, 10>(hi_word);
    u.tm_hour        = get_bits<11, 15>(hi_word);
    u.tm_mday        = get_bits< 0,  4>(lo_word);
    u.tm_mon         = get_bits< 5,  8>(lo_word) - 1;
    u.tm_year        = get_bits< 9, 15>(lo_word) + 80;
    time_t t = timegm(&u);
    return t;
}

};  // namespace LnkStruct
