#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"

#include "cetlib/compiler_macros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include "mfextensions/Receivers/detail/TCPConnect.hh"

#define TRACE_NAME "UDP_mfPlugin"
#include "trace.h"

// Boost includes
#include <boost/algorithm/string.hpp>

namespace mfplugins {
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility UDP Streamer Destination
/// Formats messages into a delimited string and sends via UDP
/// </summary>
class ELUDP : public ELdestination
{
public:
	/**
	 * \brief Configuration Parameters for ELUDP
	 */
	struct Config
	{
		/// ELDestination common config parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// "error_turnoff_threshold" (Default: 0): Number of errors before turning off destination (default: 0, don't turn
		/// off)"
		fhicl::Atom<int> error_max = fhicl::Atom<int>{
		    fhicl::Name{"error_turnoff_threshold"},
		    fhicl::Comment{"Number of errors before turning off destination (default: 0, don't turn off)"}, 0};
		/// "error_report_backoff_factor" (Default: 100): Print an error message every N errors
		fhicl::Atom<int> error_report = fhicl::Atom<int>{fhicl::Name{"error_report_backoff_factor"},
		                                                 fhicl::Comment{"Print an error message every N errors"}, 100};
		/// "host" (Default: "227.128.12.27"): Address to send messages to
		fhicl::Atom<std::string> host =
		    fhicl::Atom<std::string>{fhicl::Name{"host"}, fhicl::Comment{"Address to send messages to"}, "227.128.12.27"};
		/// "port" (Default: 5140): Port to send messages to
		fhicl::Atom<int> port = fhicl::Atom<int>{fhicl::Name{"port"}, fhicl::Comment{"Port to send messages to"}, 5140};
		/// "multicast_enabled" (Default: false): Whether messages should be sent via multicast
		fhicl::Atom<bool> multicast_enabled = fhicl::Atom<bool>{
		    fhicl::Name{"multicast_enabled"}, fhicl::Comment{"Whether messages should be sent via multicast"}, false};
		/// "multicast_interface_ip" (Default: "0.0.0.0"): Use this hostname for multicast output (to assign to the proper
		/// NIC)
		fhicl::Atom<std::string> output_address = fhicl::Atom<std::string>{
		    fhicl::Name{"multicast_interface_ip"},
		    fhicl::Comment{"Use this hostname for multicast output(to assign to the proper NIC)"}, "0.0.0.0"};
		/// filename_delimit (Default: "/"): Grab path after this. "/srcs/" /x/srcs/y/z.cc => y/z.cc
		fhicl::Atom<std::string> filename_delimit =
		    fhicl::Atom<std::string>{fhicl::Name{"filename_delimit"},
		                             fhicl::Comment{"Grab path after this. \"/srcs/\" /x/srcs/y/z.cc => y/z.cc. NOTE: only works if full filename is given to this plugin (based on which mf::<method> is used)."}, "/"};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/// <summary>
	/// ELUDP Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ELUDP</param>
	ELUDP(Parameters const& pset);

	/**
	 * \brief Fill the "Prefix" portion of the message
	 * \param o Output stringstream
	 * \param msg MessageFacility object containing header information
	 */
	void fillPrefix(std::ostringstream& o, const ErrorObj& msg) override;

	/**
	 * \brief Fill the "User Message" portion of the message
	 * \param o Output stringstream
	 * \param msg MessageFacility object containing header information
	 */
	void fillUsrMsg(std::ostringstream& o, const ErrorObj& msg) override;

	/**
	 * \brief Fill the "Suffix" portion of the message (Unused)
	 */
	void fillSuffix(std::ostringstream& /*unused*/, const ErrorObj& /*msg*/) override {}

	/**
	 * \brief Serialize a MessageFacility message to the output
	 * \param o Stringstream object containing message data
	 * \param e MessageFacility object containing header information
	 */
	void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

private:
	void reconnect_();

	// Parameters
	int error_report_backoff_factor_;
	int error_max_;
	std::string host_;
	int port_;
	bool multicast_enabled_;
	std::string multicast_out_addr_;

	int message_socket_;
	struct sockaddr_in message_addr_;

	// Other stuff
	int consecutive_success_count_;
	int error_count_;
	int next_error_report_;
	int seqNum_;

	int64_t pid_;
	std::string hostname_;
	std::string hostaddr_;
	std::string app_;
	std::string filename_delimit_;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELUDP c'tor
//======================================================================

ELUDP::ELUDP(Parameters const& pset)
    : ELdestination(pset().elDestConfig()), error_report_backoff_factor_(pset().error_report()), error_max_(pset().error_max()), host_(pset().host()), port_(pset().port()), multicast_enabled_(pset().multicast_enabled()), multicast_out_addr_(pset().output_address()), message_socket_(-1), consecutive_success_count_(0), error_count_(0), next_error_report_(1), seqNum_(0), pid_(static_cast<int64_t>(getpid())), filename_delimit_(pset().filename_delimit())
{
	// hostname
	char hostname_c[1024];
	hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";

	// host ip address
	hostent* host = nullptr;
	host = gethostbyname(hostname_c);

	if (host != nullptr)
	{
		// ip address from hostname if the entry exists in /etc/hosts
		char* ip = inet_ntoa(*reinterpret_cast<struct in_addr*>(host->h_addr));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
		hostaddr_ = ip;
	}
	else
	{
		// enumerate all network interfaces
		struct ifaddrs* ifAddrStruct = nullptr;
		struct ifaddrs* ifa = nullptr;
		void* tmpAddrPtr = nullptr;

		if (getifaddrs(&ifAddrStruct) != 0)
		{
			// failed to get addr struct
			hostaddr_ = "127.0.0.1";
		}
		else
		{
			// iterate through all interfaces
			for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
			{
				if (ifa->ifa_addr->sa_family == AF_INET)
				{
					// a valid IPv4 addres
					tmpAddrPtr = &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					char addressBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					hostaddr_ = addressBuffer;
				}

				else if (ifa->ifa_addr->sa_family == AF_INET6)
				{
					// a valid IPv6 address
					tmpAddrPtr = &(reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr)->sin6_addr);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
					hostaddr_ = addressBuffer;
				}

				// find first non-local address
				if (!hostaddr_.empty() && (hostaddr_ != "127.0.0.1") && (hostaddr_ != "::1"))
				{
					break;
				}
			}

			if (hostaddr_.empty())
			{  // failed to find anything
				hostaddr_ = "127.0.0.1";
			}
		}
	}

#if 0
		// get process name from '/proc/pid/exe'
		std::string exe;
		std::ostringstream pid_ostr;
		pid_ostr << "/proc/" << pid_ << "/exe";
		exe = realpath(pid_ostr.str().c_str(), NULL);

		size_t end = exe.find('\0');
		size_t start = exe.find_last_of('/', end);

		app_ = exe.substr(start + 1, end - start - 1);
#else
	// get process name from '/proc/pid/cmdline'
	std::stringstream ss;
	ss << "//proc//" << pid_ << "//cmdline";
	std::ifstream procfile{ss.str().c_str()};

	std::string procinfo;

	if (procfile.is_open())
	{
		procfile >> procinfo;
		procfile.close();
	}

	size_t end = procinfo.find('\0');
	size_t start = procinfo.find_last_of('/', end);

	app_ = procinfo.substr(start + 1, end - start - 1);
#endif
}

void ELUDP::reconnect_()
{
	static std::mutex mutex;
	std::lock_guard<std::mutex> lk(mutex);
	if (message_socket_ == -1)
	{
		message_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (message_socket_ < 0)
		{
			TLOG(TLVL_ERROR) << "I failed to create the socket for sending Data messages! err=" << strerror(errno);
			exit(1);
		}
		int sts = ResolveHost(host_.c_str(), port_, message_addr_);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve Data message address, err=" << strerror(errno);
			exit(1);
		}

		if (multicast_enabled_)
		{
			if (multicast_out_addr_ == "0.0.0.0")
			{
				multicast_out_addr_.reserve(HOST_NAME_MAX);
				sts = gethostname(&multicast_out_addr_[0], HOST_NAME_MAX);
				if (sts < 0)
				{
					TLOG(TLVL_ERROR) << "Could not get current hostname,  err=" << strerror(errno);
					exit(1);
				}
			}

			if (multicast_out_addr_ != "localhost")
			{
				struct in_addr addr;
				sts = GetInterfaceForNetwork(multicast_out_addr_.c_str(), addr);
				if (sts == -1)
				{
					TLOG(TLVL_ERROR) << "Unable to resolve multicast interface address, err=" << strerror(errno);
					exit(1);
				}

				if (setsockopt(message_socket_, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) == -1)
				{
					TLOG(TLVL_ERROR) << "Cannot set outgoing interface, err=" << strerror(errno);
					exit(1);
				}
			}
			int yes = 1;
			if (setsockopt(message_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
			{
				TLOG(TLVL_ERROR) << "Unable to enable port reuse on message socket, err=" << strerror(errno);
				exit(1);
			}
			if (setsockopt(message_socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &yes, sizeof(yes)) < 0)
			{
				TLOG(TLVL_ERROR) << "Unable to enable multicast loopback on message socket, err=" << strerror(errno);
				exit(1);
			}
			if (setsockopt(message_socket_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) == -1)
			{
				TLOG(TLVL_ERROR) << "Cannot set message socket to broadcast, err=" << strerror(errno);
				exit(1);
			}
		}
	}
}

//======================================================================
// Message prefix filler ( overriddes ELdestination::fillPrefix )
//======================================================================
void ELUDP::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();

	auto id = xid.id();
	auto module = xid.module();
	auto app = app_;
	std::replace(id.begin(), id.end(), '|', '!');
	std::replace(app.begin(), app.end(), '|', '!');
	std::replace(module.begin(), module.end(), '|', '!');

	oss << format_.timestamp(msg.timestamp()) << "|";  // timestamp
	oss << std::to_string(++seqNum_) << "|";           // sequence number
	oss << hostname_ << "|";                           // host name
	oss << hostaddr_ << "|";                           // host address
	oss << xid.severity().getName() << "|";            // severity
	oss << id << "|";                                  // category
	oss << app << "|";                                 // application
	oss << pid_ << "|";
	oss << mf::GetIteration() << "|";  // run/event no

	oss << module << "|";  // module name
	if (filename_delimit_.empty())
	{
		oss << msg.filename();
	}
	else if (filename_delimit_.size() == 1)  // for a single character (i.e '/'), search in reverse.
	{
		oss << (strrchr(&msg.filename()[0], filename_delimit_[0]) != nullptr  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		            ? strrchr(&msg.filename()[0], filename_delimit_[0]) + 1   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		            : msg.filename());
	}
	else
	{
		const char* cp = strstr(&msg.filename()[0], &filename_delimit_[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		if (cp != nullptr)
		{
			// make sure to remove a part that ends with '/'
			cp += filename_delimit_.size() - 1;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			while (*cp && *cp != '/') ++cp;
			++cp;  // increment past '/'
			oss << cp;
		}
		else
			oss << msg.filename();
	}
	oss << "|" << std::to_string(msg.lineNumber()) << "|";
}

//======================================================================
// Message filler ( overriddes ELdestination::fillUsrMsg )
//======================================================================
void ELUDP::fillUsrMsg(std::ostringstream& oss, const ErrorObj& msg)
{
	std::ostringstream tmposs;
	// Print the contents.
	for (auto const& val : msg.items())
	{
		tmposs << val;
	}

	// remove leading "\n" if present
	const std::string& usrMsg = tmposs.str().compare(0, 1, "\n") == 0 ? tmposs.str().erase(0, 1) : tmposs.str();

	oss << usrMsg;
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELUDP::routePayload(const std::ostringstream& oss, const ErrorObj& /*msg*/)
{
	if (message_socket_ == -1)
	{
		reconnect_();
	}
	if (error_count_ < error_max_ || error_max_ == 0)
	{
		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(message_addr_.sin_addr), str, INET_ADDRSTRLEN);

		auto string = "UDPMFMESSAGE" + std::to_string(pid_) + "|" + oss.str();
		auto sts = sendto(message_socket_, string.c_str(), string.size(), 0, reinterpret_cast<struct sockaddr*>(&message_addr_),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		                  sizeof(message_addr_));

		if (sts < 0)
		{
			consecutive_success_count_ = 0;
			++error_count_;
			if (error_count_ == next_error_report_)
			{
				TLOG(TLVL_ERROR) << "Error sending message " << seqNum_ << " to " << host_ << ", errno=" << errno << " ("
				                 << strerror(errno) << ")";
				next_error_report_ *= error_report_backoff_factor_;
			}
		}
		else
		{
			++consecutive_success_count_;
			if (consecutive_success_count_ >= 5)
			{
				error_count_ = 0;
				next_error_report_ = 1;
			}
		}
	}
}
}  // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START
auto makePlugin(const std::string& /*unused*/, const fhicl::ParameterSet& pset)
{
	return std::make_unique<mfplugins::ELUDP>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
