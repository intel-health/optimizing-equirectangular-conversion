#include "DpcppBaseAlgorithm.hpp"
#include "ConfigurableDeviceSelector.hpp"

DpcppBaseAlgorithm::DpcppBaseAlgorithm(SParameters& parameters) : BaseAlgorithm(parameters)
{
    m_pQ = NULL;
}

DpcppBaseAlgorithm::~DpcppBaseAlgorithm()
{
    delete m_pQ;
    m_pQ = NULL;
}

sycl::queue* DpcppBaseAlgorithm::GetDeviceQ()
{
    // create an async exception handler so the program fails more gracefully.
    auto ehandler = [](cl::sycl::exception_list exceptionList) {
        for (std::exception_ptr const& e : exceptionList) {
            try {
                std::rethrow_exception(e);
            }
            catch (cl::sycl::exception const& e) {
                std::cout << "Caught an asynchronous DPC++ exception, terminating the "
                    "program."
                    << std::endl;
                std::terminate();
            }
        }
    };

    try {
        if (m_pQ != NULL)
        {
            delete m_pQ;
        }
        ConfigurableDeviceSelector::set_search(m_typePreference, m_platformName, m_deviceName, m_driverVersion);
        m_pQ = new sycl::queue(ConfigurableDeviceSelector::device_selector, ehandler);
#define SHOW_RESULTS
#ifdef SHOW_RESULTS
        sycl::device device = m_pQ->get_device();
        ConfigurableDeviceSelector::print_platform_info(device);
        ConfigurableDeviceSelector::print_device_info(device);
#endif
    }
    catch (cl::sycl::exception const& e) {
        m_pQ = NULL;
    }

    return m_pQ;
}

sycl::queue* DpcppBaseAlgorithm::GetNextDeviceQ()
{
    bool bDone = false;

    do
    {
        if (m_curDevice != m_devices.end())
        {
            m_curDevice++;
        }
        if (m_curDevice == m_devices.end())
        {
            if (m_curPlatform != m_platforms.end())
            {
                m_curPlatform++;
            }
            if (m_curPlatform != m_platforms.end())
            {
                m_devices = m_curPlatform->get_devices();
                m_curDevice = m_devices.begin();
            }
        }
        if (m_curPlatform != m_platforms.end() && m_curDevice != m_devices.end())
        {
            m_platformName = m_curPlatform->get_info<sycl::info::platform::name>();
            m_deviceName = m_curDevice->get_info<sycl::info::device::name>();
            m_driverVersion = m_curDevice->get_info<sycl::info::device::driver_version>();
            GetDeviceQ();
        }
        else
        {
            bDone = true;
            delete m_pQ;
            m_pQ = NULL;
        }
    } while (m_pQ == NULL && !bDone);

    return m_pQ;
}

bool DpcppBaseAlgorithm::StartSubAlgorithm(uint algNum)
{
    if (algNum == 0)
    {
        // First time, so we need to figure out if this is a one-shot run (i.e.,
        // neither the m_platformName nor m_deviceName == all or if we will be looping
        // due to either being all.
        if (m_parameters->m_platformName == "all" || m_parameters->m_deviceName == "all")
        {
            m_platforms = sycl::platform::get_platforms();
            m_curPlatform = m_platforms.begin();
            if (m_curPlatform != m_platforms.end())
            {
                m_devices = m_curPlatform->get_devices();
                m_curDevice = m_devices.begin();
            }
            m_typePreference = m_parameters->m_typePreference;
            m_platformName = m_curPlatform->get_info<sycl::info::platform::name>();
            m_deviceName = m_curDevice->get_info<sycl::info::device::name>();
            m_driverVersion = m_curDevice->get_info<sycl::info::device::driver_version>();
            m_bAll = true;
        }
        else
        {
            m_typePreference = m_parameters->m_typePreference;
            m_platformName = m_parameters->m_platformName;
            m_deviceName = m_parameters->m_deviceName;
            m_driverVersion = m_parameters->m_driverVersion;

            m_bAll = false;
        }
        if (GetDeviceQ() == NULL)
        {
            GetNextDeviceQ();
        }
    }
    else
    {
        if (m_bAll)
        {
            GetNextDeviceQ();
        }
        else
        {
            delete m_pQ;
            m_pQ = NULL;
        }
    }

    return m_pQ != NULL;
}

std::string DpcppBaseAlgorithm::GetDeviceDescription()
{
    std::string retVal = "";

    if (m_pQ != NULL)
    {
        retVal = ConfigurableDeviceSelector::get_device_description(m_pQ->get_device());
    }

    return retVal;
}

