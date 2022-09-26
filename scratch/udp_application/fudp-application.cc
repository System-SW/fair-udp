#include "fudp-application.h"

#include "ns3/abort.h"
#include "ns3/callback.h"
#include "ns3/fatal-error.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/type-id.h"

FudpApplication &FudpApplicationImpl::GetContainer ()
{
  return *_container;
}

void FudpApplicationImpl::SetContainer (FudpApplication *newContainer)
{
  _container = newContainer;
}

void FudpApplication::SetImpl (::std::shared_ptr<FudpApplicationImpl> impl)
{
  if (_impl != nullptr)
    {
      _impl->SetContainer (nullptr);
    }

  _impl = impl;
  _impl->SetContainer (this);
}

void FudpApplication::StartApplication ()
{
  NS_ABORT_IF (_impl == nullptr);
  _impl->StartApplication ();
}

void FudpApplication::StopApplication ()
{
  if (_impl != nullptr)
    {
      _impl->StopApplication ();
    }
}

::ns3::TypeId FudpApplication::GetInstanceTypeId () const
{
  return GetTypeId ();
}

::ns3::TypeId FudpApplication::GetTypeId ()
{
  static auto const tid =
      ::ns3::TypeId{"ns3::FudpApplication"}.AddConstructor<FudpApplication> ().SetParent<::ns3::Application> ();
  return tid;
}
