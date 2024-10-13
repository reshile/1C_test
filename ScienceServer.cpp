#include "message.hpp"
#include "server.hpp"
#include <boost/asio/execution/set_error.hpp>
#include <boost/date_time/gregorian/greg_day_of_year.hpp>

class ScienceServer : public ServerInterface {
  std::map<uint32_t, uint32_t> leaderboard;
  int64_t current_number;
  bool active_experiment = false;

 public:
  
  ScienceServer(uint16_t port) : ServerInterface(port) {} 
  
  void StartExperiment() {
    Message msg{MessageType::Broadcast, 0};
    BroadCast(msg);
  }

  void FinishExperiment() {
    Message msg{MessageType::FinishExperiment, 0};
    BroadCast(msg);
  }

  void HandleMessage(std::shared_ptr<Connection> client, Message& msg) {
    if (msg.ms_type == MessageType::Response) {
      if (msg.number == current_number) {
        Message resp{MessageType::Equals, 0};
        ++leaderboard[client->getId()];
        client->SendMessage(resp);
      } else if (msg.number < current_number) {
        Message resp{MessageType::Less, 0};
        client->SendMessage(resp);
      } else if (msg.number > current_number) {
        Message resp{MessageType::Greater, 0};
        client->SendMessage(resp);
      }
    }

    std::cout << "To finish experiment enter 1\n"
              << "anythin else to continue\n";
    int ans;
    std::cin >> ans;
    if (ans == 1) {
      active_experiment = false;
      FinishExperiment();
    }
  }

  void PrintLeaders() {
    for (auto const& [key, value] : leaderboard) {
      std::cout << "User " << key << " scored " << value << '\n';
    }
  }

  void DoAction() {
    if (!active_experiment) {
      std::cout << "To start new experiment enter 0\n"
                << "anything else to skip\n";
      int ans;
      std::cin >> ans;

      std::cout << "Enter guessing number: ";
      std::cin >> current_number;

      active_experiment = true;
      if (ans == 0) {
        StartExperiment();
      }
    }    
  }
 };


int main() {
  uint16_t port = 60000;
  std::cout << "Enter valid port: ";
  std::cin >> port;
  ScienceServer serv(port);  

  serv.Start();
  while (true) {
    serv.DoAction();
    serv.ReadMessages();
  }
}
