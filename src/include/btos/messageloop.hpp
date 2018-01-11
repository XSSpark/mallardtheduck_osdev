#ifndef _MESSAGELOOP_HPP
#define _MESSAGELOOP_HPP

#include <btos.h>
#include "imessagehandler.hpp"
#include "message.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace btos_api{
	
	class MessageLoop : public IMessageHandler{
	private:
		std::unique_ptr<Message> current;
		std::vector<std::shared_ptr<IMessageHandler>> handlers;
		std::function<bool(const Message&)> criticalHandler;
		std::function<bool(const Message&)> previewer;
	public:
		MessageLoop() = default;
		void AddHandler(std::shared_ptr<IMessageHandler> h);
		void RemoveHandler(std::shared_ptr<IMessageHandler> h);

		void SetCriticalHandler(std::function<bool(const Message&)> fn);
		std::function<bool(const Message&)> GetCriticalHandler();

		void SetPreviewer(std::function<bool(const Message&)> fn);
		std::function<bool(const Message&)> GetPreviewer();

		void RunLoop();
		bool HandleMessage(const Message &msg) override;
		const Message &GetCurrent() const;
	};
	
}

#endif
