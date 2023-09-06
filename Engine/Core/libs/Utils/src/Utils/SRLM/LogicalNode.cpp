//
// Created by Monika on 30.08.2023.
//

#include <Utils/SRLM/LogicalNode.h>

namespace SR_SRLM_NS {
    LogicalNode::~LogicalNode() {
        for (auto&& pin : m_inputs) {
            delete pin.pData;
        }

        for (auto&& pin : m_outputs) {
            delete pin.pData;
        }
    }

    LogicalNode* LogicalNode::LoadXml(const SR_XML_NS::Node& xmlNode) {
        auto&& hashName = xmlNode.GetAttribute("Name").ToUInt64();

        auto&& pLogicalNode = SR_SRLM_NS::LogicalNodeManager::Instance().CreateByName(hashName);
        if (!pLogicalNode) {
            SR_ERROR("LogicalNode::LoadXml() : failed to load node!\n\tHash name: " + SR_UTILS_NS::ToString(hashName));
            return nullptr;
        }

        for (auto&& xmlPin : xmlNode.GetNodes()) {
            auto&& pDataType = SR_SRLM_NS::DataType::LoadXml(xmlPin.GetNode("DT"));
            if (!pDataType) {
                SR_ERROR("LogicalNode::LoadXml() : failed to load pin!\n\tName: " + xmlPin.Name());
                continue;
            }

            auto&& pinHashName = SR_HASH_STR_REGISTER(xmlPin.GetAttribute("Name").ToString());

            if (xmlPin.NameView() == "Input") {
                pLogicalNode->AddInputData(pDataType, pinHashName);
            }
            else if (xmlPin.NameView() == "Output") {
                pLogicalNode->AddOutputData(pDataType, pinHashName);
            }
            else {
                SR_ERROR("LogicalNode::LoadXml() : invalid pin name!\n\tName: " + xmlPin.Name());
            }
        }

        return pLogicalNode;
    }

    void LogicalNode::SaveXml(SR_XML_NS::Node& xmlNode) {
        xmlNode.AppendAttribute("Name", GetHashName());

        for (auto&& pin : GetInputs()) {
            auto&& xmlPinNode = xmlNode.AppendNode("Input");
            xmlPinNode.AppendAttribute("Name", SR_HASH_TO_STR(pin.hashName));

            auto&& xmlDataType = xmlPinNode.AppendNode("DT");
            pin.pData->SaveXml(xmlDataType);
        }

        for (auto&& pin : GetOutputs()) {
            auto&& xmlPinNode = xmlNode.AppendNode("Output");
            xmlPinNode.AppendAttribute("Name", SR_HASH_TO_STR(pin.hashName));

            auto&& xmlDataType = xmlPinNode.AppendNode("DT");
            pin.pData->SaveXml(xmlDataType);
        }
    }

    void LogicalNode::SetInput(const DataType* pInput, uint8_t index) {
        if (!pInput) {
            m_status |= LogicalNodeStatus::InputNullPtr;
            return;
        }

        if (index >= m_inputs.size()) {
            m_status |= LogicalNodeStatus::InputRangeError;
            return;
        }

        auto&& pSelfInput = m_inputs.at(index).pData;
        if (pSelfInput->GetMeta() != pInput->GetMeta()) {
            m_status |= LogicalNodeStatus::InputTypeError;
            return;
        }

        pInput->CopyTo(pSelfInput);
    }

    void LogicalNode::Reset() {
        for (auto&& pin : m_inputs) {
            pin.pData->Reset();
        }

        for (auto&& pin : m_outputs) {
            pin.pData->Reset();
        }

        m_status = LogicalNodeStatus::None;
    }

    const DataType* LogicalNode::GetOutput(uint8_t index) {
        if (index >= m_inputs.size()) {
            m_status |= LogicalNodeStatus::OutputRangeError;
            return nullptr;
        }

        if (m_status & LogicalNodeStatus::ErrorStatus) {
            return nullptr;
        }

        return m_inputs.at(index).pData;
    }

    void LogicalNode::MarkDirty() {
        if (GetType() == LogicalNodeType::Compute || GetType() == LogicalNodeType::Connector) {
            for (auto&& pin : m_outputs) {
                pin.pNode->MarkDirty();
            }
        }
    }

    LogicalNode* LogicalNode::GetOutputNode(uint8_t index) {
        if (index >= m_outputs.size()) {
            m_status |= LogicalNodeStatus::OutputRangeError;
            return nullptr;
        }

        return m_outputs.at(index).pNode;
    }

    void LogicalNode::AddInputData(DataType* pData, uint64_t hashName) {
        auto&& pin = NodePin();
        pin.pData = pData;
        pin.hashName = hashName;
        m_inputs.emplace_back(pin);
    }

    void LogicalNode::AddOutputData(DataType* pData, uint64_t hashName) {
        auto&& pin = NodePin();
        pin.pData = pData;
        pin.hashName = hashName;
        m_outputs.emplace_back(pin);
    }

    /// ----------------------------------------------------------------------------------------------------------------

    const DataType* IComputeNode::GetOutput(uint8_t index) {
        if (m_dirty)
        {
            Compute();

            if (HasErrors()) {
                m_dirty = false;
                m_status ^= LogicalNodeStatus::ComputeError;
            }
            else {
                m_status |= LogicalNodeStatus::ComputeError;
                return nullptr;
            }
        }

        return LogicalNode::GetOutput(index);
    }

    void IComputeNode::MarkDirty() {
        m_dirty = true;
        LogicalNode::MarkDirty();
    }
}