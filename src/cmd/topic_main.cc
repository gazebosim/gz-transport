#include <ignition/utils/cli/CLI.hpp>

#include "ign.hh"

#include <ignition/transport/config.hh>

//////////////////////////////////////////////////
/// \brief Enumeration of available commands
enum class TopicCommand
{
  kNone,
  kTopicList,
  kTopicInfo,
  kTopicPub,
  kTopicEcho
};

//////////////////////////////////////////////////
/// \brief Structure to hold all available topic options
struct TopicOptions
{
  /// \brief Command to execute
  TopicCommand command{TopicCommand::kNone};

  /// \brief Name of the topic
  std::string topic{""};

  /// \brief Message type to use when publishing
  std::string msgType{""};

  /// \brief Message data to use when publishing
  std::string msgData{""};

  /// \brief Amount of time to echo (in seconds)
  double duration{-1};

  /// \brief Number of messages to echo 
  int count{-1};
};

//////////////////////////////////////////////////
/// \brief Callback fired when options are successfully parsed
void runTopicCommand(const TopicOptions &_opt)
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
    case TopicCommand::kNone:
    default:
      // In the event that there is no command, display help
      throw CLI::CallForHelp();
  }
}

//////////////////////////////////////////////////
void addTopicFlags(CLI::App &_app)
{
  auto opt = std::make_shared<TopicOptions>();

  auto topicOpt = _app.add_option("-t,--topic", 
                                  opt->topic, "Name of a topic");
  auto msgTypeOpt = _app.add_option("-m,--msgtype", 
                                    opt->msgType, "Type of message to publish");
  auto durationOpt = _app.add_option("-d,--duration", 
                                     opt->duration, 
                                     "Duration (seconds) to run");
  auto countOpt = _app.add_option("-n,--num", 
                                  opt->count, 
                                  "Numer of messages to echo and then exit");

  durationOpt->excludes(countOpt);
  countOpt->excludes(durationOpt);

  auto command = _app.add_option_group("command", "Command to be executed");

  command->add_flag_callback("-l,--list", 
    [opt](){ 
      opt->command = TopicCommand::kTopicList; 
    });

  command->add_flag_callback("-i,--info", 
    [opt](){ 
      opt->command = TopicCommand::kTopicInfo; 
    })
    ->needs(topicOpt);

  command->add_flag_callback("-e,--echo", 
    [opt](){
      opt->command = TopicCommand::kTopicEcho; 
    });

  command->add_option_function<std::string>("-p,--pub",
      [&](const std::string &_msgData){
        opt->command = TopicCommand::kTopicPub;
        opt->msgData = _msgData;
      })
    ->needs(topicOpt)
    ->needs(msgTypeOpt);

  _app.callback([opt](){runTopicCommand(*opt); });
}

//////////////////////////////////////////////////
int main(int argc, char** argv)
{
  CLI::App app{"Introspect Ignition topics"};

  app.add_flag_callback("-v,--version", [](){
      std::cout << IGNITION_TRANSPORT_VERSION_FULL << std::endl;
      throw CLI::Success();
  });

  addTopicFlags(app);
  CLI11_PARSE(app, argc, argv);
}
