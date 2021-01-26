#include <ignition/utils/cli/CLI.hpp>

#include "ign.hh"

#include <ignition/transport/config.hh>

//////////////////////////////////////////////////
/// \brief Enumeration of available commands
enum class TopicCommand
{
  kTopicList,
  kTopicInfo,
  kTopicPub,
  kTopicEcho 
};

//////////////////////////////////////////////////
struct TopicOptions
{
  TopicCommand command;
  std::string topic{""};
  std::string msgType{""};
  std::string msgData{""};
  double duration{-1};
  int count{-1};
};

//////////////////////////////////////////////////
void run_subcommand_topic(const TopicOptions &_opt)
{
  switch(_opt.command)
  {
    case TopicCommand::kTopicList:
      cmdTopicList();
      break;
    case TopicCommand::kTopicInfo:
      cmdTopicInfo(_opt.topic.c_str());
      break;
    case TopicCommand::kTopicPub:
      cmdTopicPub(_opt.topic.c_str(), 
                  _opt.msgType.c_str(), 
                  _opt.msgData.c_str());
      break;
    case TopicCommand::kTopicEcho:
      cmdTopicEcho(_opt.topic.c_str(), _opt.duration, _opt.count);
      break;
    default:
      break;
  }
}

//////////////////////////////////////////////////
void add_subcommand_topic(CLI::App &_app)
{
  CLI::App* sub = _app.add_subcommand("topic", "Introspect ignition topics");
  auto opt = std::make_shared<TopicOptions>();

  auto topicOpt = sub->add_option("-t,--topic", opt->topic, "Name of a topic");
  auto msgTypeOpt = sub->add_option("-m,--msgtype", opt->msgType, "Type of message to publish");
  auto durationOpt = sub->add_option("-d,--duration", opt->duration, "Duration (seconds) to run");
  auto countOpt = sub->add_option("-n,--num", opt->count, "Numer of messages to echo and then exit");

  durationOpt->excludes(countOpt);
  countOpt->excludes(durationOpt);

  // Use an option group to guarantee exactly one command choosen
  auto command = sub->add_option_group("command", "Command to be executed");
  command->add_flag_callback("-l,--list", [&](){ opt->command = TopicCommand::kTopicList; });
  command->add_flag_callback("-i,--info", [&](){ opt->command = TopicCommand::kTopicInfo; })
    ->needs(topicOpt);
  command->add_flag_callback("-e,--echo", [&](){ opt->command = TopicCommand::kTopicEcho; });
  command->add_option_function<std::string>("-p,--pub", 
      [&](const std::string &_msgData){ 
        opt->command = TopicCommand::kTopicPub; 
        opt->msgData = _msgData;
      })
    ->needs(topicOpt)
    ->needs(msgTypeOpt);
  command->require_option(1);

  sub->callback([opt](){run_subcommand_topic(*opt); });
}


//////////////////////////////////////////////////
/// \brief Enumeration of available commands
enum class ServiceCommand
{
  kServiceList,
  kServiceInfo,
  kServiceReq,
};

//////////////////////////////////////////////////
struct ServiceOptions
{
  ServiceCommand command;
  std::string service{""};
  std::string reqData{""};
  std::string reqType{""};
  std::string repType{""};
  int timeout{-1};
};

//////////////////////////////////////////////////
void run_subcommand_service(const ServiceOptions &_opt)
{
  switch(_opt.command)
  {
    case ServiceCommand::kServiceList:
      cmdServiceList();
      break;
    case ServiceCommand::kServiceInfo:
      cmdServiceInfo(_opt.service.c_str());
      break;
    case ServiceCommand::kServiceReq:
      cmdServiceReq(_opt.service.c_str(), 
          _opt.reqType.c_str(), _opt.repType.c_str(),
          _opt.timeout, _opt.reqData.c_str());
      break;
    default:
      break;
  }
}

//////////////////////////////////////////////////
void add_subcommand_service(CLI::App &_app)
{
  CLI::App* sub = _app.add_subcommand("service", "Introspect ignition services");
  auto opt = std::make_shared<ServiceOptions>();
  sub->callback([opt](){run_subcommand_service(*opt); });

  auto serviceOpt = sub->add_option("-s,--service", opt->service, "Name of a service");
  auto reqTypeOpt = sub->add_option("--reqtype", opt->reqType, "Type of a request.");
  auto repTypeOpt = sub->add_option("--reptype", opt->repType, "Type of a response.");
  auto timeoutOpt = sub->add_option("--timeout", opt->timeout, "Timeout in milliseconds.");

  // Use an option group to guarantee exactly one command choosen
  auto command = sub->add_option_group("command", "Command to be executed");
  command->add_flag_callback("-l,--list", [&](){ opt->command = ServiceCommand::kServiceList; });
  command->add_flag_callback("-i,--info", [&](){ opt->command = ServiceCommand::kServiceInfo; })
    ->needs(serviceOpt);
  command->add_option_function<std::string>("-r,--req", 
      [&](const std::string &_reqData){ 
        opt->command = ServiceCommand::kServiceReq; 
        opt->reqData = _reqData;
      })
    ->needs(serviceOpt)
    ->needs(reqTypeOpt)
    ->needs(repTypeOpt)
    ->needs(timeoutOpt);
  command->require_option(1);

  sub->callback([opt](){run_subcommand_service(*opt); });
}

//////////////////////////////////////////////////
int main(int argc, char** argv)
{
  CLI::App app{"transport"};

  app.set_help_all_flag("--help-all", "Show all help");

  app.add_flag_callback("-v,--version", [](){
      std::cout << IGNITION_TRANSPORT_VERSION_FULL << std::endl;
      throw CLI::Success();
  });

  add_subcommand_topic(app);
  add_subcommand_service(app);
  app.require_subcommand();

  CLI11_PARSE(app, argc, argv);
}
