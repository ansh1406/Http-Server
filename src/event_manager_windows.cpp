#ifdef _WIN32

#include "event_manager.hpp"
#include "wepoll/wepoll.h"

#include <winsock2.h>

namespace
{
	std::string wsa_error_message(const std::string &prefix)
	{
		return prefix + std::to_string(WSAGetLastError());
	}
}

namespace tcp
{

	struct EventManager::Impl
	{
		HANDLE epoll_handle;

		Impl() : epoll_handle(nullptr) {}
	};

	EventManager::EventManager(const int max_events, const time_t timeout) : max_events(max_events), timeout(timeout)
	{
		pimpl = new Impl();
		HANDLE handle = epoll_create1(0);
		if (handle == nullptr)
		{
			throw exceptions::CanNotCreateEventManager(wsa_error_message("Failed to create epoll instance: "));
		}
		pimpl->epoll_handle = handle;
	}

	EventManager::EventManager(EventManager &&other) noexcept
		: pimpl(other.pimpl), max_events(other.max_events), timeout(other.timeout)
	{
		other.pimpl = nullptr;
	}

	EventManager &EventManager::operator=(EventManager &&other) noexcept
	{
		if (this != &other)
		{
			if (pimpl && pimpl->epoll_handle)
				epoll_close(pimpl->epoll_handle);

			pimpl->epoll_handle = other.pimpl->epoll_handle;
			status = std::move(other.status);
			max_events = other.max_events;
			timeout = other.timeout;
			other.pimpl->epoll_handle = nullptr;
		}
		return *this;
	}

	int EventManager::register_for_read(const int fd)
	{
		try
		{
			epoll_event ev{};
			ev.events = EPOLLIN;
			ev.data.fd = fd;

			if (epoll_ctl(pimpl->epoll_handle, EPOLL_CTL_ADD, static_cast<SOCKET>(fd), &ev) != 0)
			{
				throw exceptions::CanNotRegisterSocket(wsa_error_message("Failed to register socket: "));
			}

			status[fd] = socket_status::WRITABLE;
			return fd;
		}
		catch (exceptions::CanNotRegisterSocket &)
		{
			throw;
		}
		catch (std::exception &e)
		{
			throw exceptions::CanNotRegisterSocket("Failed to register socket: " + std::string(e.what()));
		}
		catch (...)
		{
			throw exceptions::CanNotRegisterSocket("Failed to register socket: Unknown error");
		}
	}

	int EventManager::register_for_write(const int fd)
	{
		try
		{
			epoll_event ev{};
			ev.events = EPOLLOUT;
			ev.data.fd = fd;

			if (epoll_ctl(pimpl->epoll_handle, EPOLL_CTL_ADD, static_cast<SOCKET>(fd), &ev) != 0)
			{
				throw exceptions::CanNotRegisterSocket(wsa_error_message("Failed to register socket: "));
			}

			status[fd] = socket_status::WRITABLE;
			return fd;
		}
		catch (exceptions::CanNotRegisterSocket &)
		{
			throw;
		}
		catch (std::exception &e)
		{
			throw exceptions::CanNotRegisterSocket("Failed to register socket: " + std::string(e.what()));
		}
		catch (...)
		{
			throw exceptions::CanNotRegisterSocket("Failed to register socket: Unknown error");
		}
	}

	void EventManager::remove_socket(const int id)
	{
		try
		{
			if (epoll_ctl(pimpl->epoll_handle, EPOLL_CTL_DEL, static_cast<SOCKET>(id), nullptr) != 0)
			{
				throw exceptions::CanNotRemoveSocket(wsa_error_message("Failed to remove socket: "));
			}

			status.erase(id);
		}
		catch (exceptions::CanNotRemoveSocket &)
		{
			throw;
		}
		catch (std::exception &e)
		{
			throw exceptions::CanNotRemoveSocket("Failed to remove socket: " + std::string(e.what()));
		}
		catch (...)
		{
			throw exceptions::CanNotRemoveSocket("Failed to remove socket: Unknown error");
		}
	}

	EventManager::~EventManager()
	{
		if (pimpl)
		{
			if (pimpl->epoll_handle)
				epoll_close(pimpl->epoll_handle);
		}

		delete pimpl;
	}

	std::vector<int> EventManager::wait_for_events()
	{
		try
		{
			std::vector<epoll_event> events(max_events);
			int num_events = epoll_wait(pimpl->epoll_handle, events.data(), max_events, static_cast<int>(timeout));
			if (num_events == -1)
			{
				throw exceptions::CanNotWaitForEvents(wsa_error_message("Failed to wait for events: "));
			}

			std::vector<int> fds;
			fds.reserve(static_cast<size_t>(num_events));

			for (int i = 0; i < num_events; ++i)
			{
				int fd = events[i].data.fd;
				if (events[i].events & EPOLLIN)
				{
					status[fd] |= socket_status::READABLE;
				}
				if (events[i].events & EPOLLOUT)
				{
					status[fd] |= socket_status::WRITABLE;
				}
				fds.push_back(fd);
			}

			return fds;
		}
		catch (exceptions::CanNotWaitForEvents &)
		{
			throw;
		}
		catch (std::exception &e)
		{
			throw exceptions::CanNotWaitForEvents("Failed to wait for events: " + std::string(e.what()));
		}
		catch (...)
		{
			throw exceptions::CanNotWaitForEvents("Failed to wait for events: Unknown error");
		}
	}

	const bool EventManager::is_readable(const int id) const
	{
		return status.at(id) & socket_status::READABLE;
	}

	const bool EventManager::is_writable(const int id) const
	{
		return status.at(id) & socket_status::WRITABLE;
	}

	void EventManager::clear_status(const int id)
	{
		status[id] = socket_status::IDLE;
	}
}

#endif
