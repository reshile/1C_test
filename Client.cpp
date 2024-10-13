#include "client_interface.hpp"
#include "message.hpp"
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/detail/config.hpp>

class Client : public ClientInterface {
  std::vector<int64_t> queries;
  bool active_experiment = false;

 private:

  void Guess() {
    int64_t number;
    // no checks provided :(
    std::cout << "Please, enter a number: ";
    std::cin >> number;

    queries.push_back(number);

    Message msg{MessageType::Response, number};
    SendMessage(msg);
  }

  void HandleBroadcast() {
    std::cout << "New experiment started!\n";
    active_experiment = true;
    Guess();
    Ask();
  }

  void HandleEquals() {
    std::cout << "You guessed number right!\n";
    Ask();
  }

  void HandleGreater() {
    std::cout << "You guessed greater numer, try again\n";
    Guess();
    Ask();
  }

  void HandleLess() {
    std::cout << "You guessed less number, try again\n";
    Guess();    
    Ask();
  }

  void HandleFinish() {
    std::cout << "Experiment finished!\n";
    active_experiment = false;
    queries.clear();
  }

  void Ask() {
    std::cout << "Type 1 to see previous results\n" 
              << "-1 to continue: ";
    int flag;
    std::cin >> flag;

    if (flag == 1) {
      PrintPreviousResults();
     }
  }
  
  void PrintPreviousResults() {
    std::cout << "Your previous answers: \n";
    for (auto& query : queries) {
      std::cout << query << " ";
    }
  }
  
 public:

  void WaitExperiment() {
    if (!active_experiment) {
      // wait for broadcast
      std::cout << "No experiment started, wait...\n";
      GetInputMessages().wait();
      active_experiment = true;
    }
  }

  void CheckIncoming() {
    if (ConnectionValid()) {
      if (!GetInputMessages().empty()) {
        auto con_msg = GetInputMessages().pop_front();
        
        switch (con_msg.msg.ms_type) {
          case MessageType::Broadcast:
            HandleBroadcast();
            break;
          case MessageType::Equals:
            HandleEquals();
            break;
          case MessageType::Greater:
            HandleGreater();
            break;
          case MessageType::Less:
            break;
          case MessageType::FinishExperiment:
            HandleFinish();
            break;
          default:
          break;
        }
      }
    }
  }
};

int main() {
  Client client;

  std::string host;
  uint16_t port;

  std::cout << "Enter host: ";
  std::cin >> host;

  std::cout << "Enter port: ";
  std::cin >> port;

  client.Connect(host, port);

  bool active = true;

  while (active) {
    client.WaitExperiment(); 
    client.CheckIncoming();
  }
  
}
