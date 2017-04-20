struct Array_VkDescriptorSetLayout {
	VkDescriptorSetLayout* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkDescriptorSetLayout& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkDescriptorSetLayoutBinding {
	VkDescriptorSetLayoutBinding* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkDescriptorSetLayoutBinding& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkVertexInputBindingDescription {
	VkVertexInputBindingDescription* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkVertexInputBindingDescription& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkVertexInputAttributeDescription {
	VkVertexInputAttributeDescription* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkVertexInputAttributeDescription& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkPipelineShaderStageCreateInfo {
	VkPipelineShaderStageCreateInfo* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkPipelineShaderStageCreateInfo& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkDescriptorPoolSize {
	VkDescriptorPoolSize* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkDescriptorPoolSize& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkAttachmentDescription {
	VkAttachmentDescription* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkAttachmentDescription& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_VkAttachmentReference {
	VkAttachmentReference* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	VkAttachmentReference& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_Entity {
	Entity* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	Entity& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_Vector3 {
	Vector3* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	Vector3& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_i32 {
	i32* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	i32& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_RenderObject {
	RenderObject* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	RenderObject& operator[] (isize i)
	{
		return data[i];
	}
};

struct Array_IndexRenderObject {
	IndexRenderObject* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	IndexRenderObject& operator[] (isize i)
	{
		return data[i];
	}
};

