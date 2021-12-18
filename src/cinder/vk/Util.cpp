#include "cinder/vk/Util.h"
#include "cinder/vk/Device.h"

namespace cinder::vk {

// Set up data structure with size(bytes) and number of components for each Vulkan format.
// For compressed and multi-plane formats, size is bytes per compressed or shared block.
//
struct FormatInfo
{
	uint32_t size;
	uint32_t componentCount;
};

// clang-format off
const std::map<VkFormat, FormatInfo> kFormatInfo = {
    {VK_FORMAT_UNDEFINED,                   { 0, 0}},
    {VK_FORMAT_R4G4_UNORM_PACK8,            { 1, 2}},
    {VK_FORMAT_R4G4B4A4_UNORM_PACK16,       { 2, 4}},
    {VK_FORMAT_B4G4R4A4_UNORM_PACK16,       { 2, 4}},
    {VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT,   { 2, 4}},
    {VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT,   { 2, 4}},
    {VK_FORMAT_R5G6B5_UNORM_PACK16,         { 2, 3}},
    {VK_FORMAT_B5G6R5_UNORM_PACK16,         { 2, 3}},
    {VK_FORMAT_R5G5B5A1_UNORM_PACK16,       { 2, 4}},
    {VK_FORMAT_B5G5R5A1_UNORM_PACK16,       { 2, 4}},
    {VK_FORMAT_A1R5G5B5_UNORM_PACK16,       { 2, 4}},
    {VK_FORMAT_R8_UNORM,                    { 1, 1}},
    {VK_FORMAT_R8_SNORM,                    { 1, 1}},
    {VK_FORMAT_R8_USCALED,                  { 1, 1}},
    {VK_FORMAT_R8_SSCALED,                  { 1, 1}},
    {VK_FORMAT_R8_UINT,                     { 1, 1}},
    {VK_FORMAT_R8_SINT,                     { 1, 1}},
    {VK_FORMAT_R8_SRGB,                     { 1, 1}},
    {VK_FORMAT_R8G8_UNORM,                  { 2, 2}},
    {VK_FORMAT_R8G8_SNORM,                  { 2, 2}},
    {VK_FORMAT_R8G8_USCALED,                { 2, 2}},
    {VK_FORMAT_R8G8_SSCALED,                { 2, 2}},
    {VK_FORMAT_R8G8_UINT,                   { 2, 2}},
    {VK_FORMAT_R8G8_SINT,                   { 2, 2}},
    {VK_FORMAT_R8G8_SRGB,                   { 2, 2}},
    {VK_FORMAT_R8G8B8_UNORM,                { 3, 3}},
    {VK_FORMAT_R8G8B8_SNORM,                { 3, 3}},
    {VK_FORMAT_R8G8B8_USCALED,              { 3, 3}},
    {VK_FORMAT_R8G8B8_SSCALED,              { 3, 3}},
    {VK_FORMAT_R8G8B8_UINT,                 { 3, 3}},
    {VK_FORMAT_R8G8B8_SINT,                 { 3, 3}},
    {VK_FORMAT_R8G8B8_SRGB,                 { 3, 3}},
    {VK_FORMAT_B8G8R8_UNORM,                { 3, 3}},
    {VK_FORMAT_B8G8R8_SNORM,                { 3, 3}},
    {VK_FORMAT_B8G8R8_USCALED,              { 3, 3}},
    {VK_FORMAT_B8G8R8_SSCALED,              { 3, 3}},
    {VK_FORMAT_B8G8R8_UINT,                 { 3, 3}},
    {VK_FORMAT_B8G8R8_SINT,                 { 3, 3}},
    {VK_FORMAT_B8G8R8_SRGB,                 { 3, 3}},
    {VK_FORMAT_R8G8B8A8_UNORM,              { 4, 4}},
    {VK_FORMAT_R8G8B8A8_SNORM,              { 4, 4}},
    {VK_FORMAT_R8G8B8A8_USCALED,            { 4, 4}},
    {VK_FORMAT_R8G8B8A8_SSCALED,            { 4, 4}},
    {VK_FORMAT_R8G8B8A8_UINT,               { 4, 4}},
    {VK_FORMAT_R8G8B8A8_SINT,               { 4, 4}},
    {VK_FORMAT_R8G8B8A8_SRGB,               { 4, 4}},
    {VK_FORMAT_B8G8R8A8_UNORM,              { 4, 4}},
    {VK_FORMAT_B8G8R8A8_SNORM,              { 4, 4}},
    {VK_FORMAT_B8G8R8A8_USCALED,            { 4, 4}},
    {VK_FORMAT_B8G8R8A8_SSCALED,            { 4, 4}},
    {VK_FORMAT_B8G8R8A8_UINT,               { 4, 4}},
    {VK_FORMAT_B8G8R8A8_SINT,               { 4, 4}},
    {VK_FORMAT_B8G8R8A8_SRGB,               { 4, 4}},
    {VK_FORMAT_A8B8G8R8_UNORM_PACK32,       { 4, 4}},
    {VK_FORMAT_A8B8G8R8_SNORM_PACK32,       { 4, 4}},
    {VK_FORMAT_A8B8G8R8_USCALED_PACK32,     { 4, 4}},
    {VK_FORMAT_A8B8G8R8_SSCALED_PACK32,     { 4, 4}},
    {VK_FORMAT_A8B8G8R8_UINT_PACK32,        { 4, 4}},
    {VK_FORMAT_A8B8G8R8_SINT_PACK32,        { 4, 4}},
    {VK_FORMAT_A8B8G8R8_SRGB_PACK32,        { 4, 4}},
    {VK_FORMAT_A2R10G10B10_UNORM_PACK32,    { 4, 4}},
    {VK_FORMAT_A2R10G10B10_SNORM_PACK32,    { 4, 4}},
    {VK_FORMAT_A2R10G10B10_USCALED_PACK32,  { 4, 4}},
    {VK_FORMAT_A2R10G10B10_SSCALED_PACK32,  { 4, 4}},
    {VK_FORMAT_A2R10G10B10_UINT_PACK32,     { 4, 4}},
    {VK_FORMAT_A2R10G10B10_SINT_PACK32,     { 4, 4}},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32,    { 4, 4}},
    {VK_FORMAT_A2B10G10R10_SNORM_PACK32,    { 4, 4}},
    {VK_FORMAT_A2B10G10R10_USCALED_PACK32,  { 4, 4}},
    {VK_FORMAT_A2B10G10R10_SSCALED_PACK32,  { 4, 4}},
    {VK_FORMAT_A2B10G10R10_UINT_PACK32,     { 4, 4}},
    {VK_FORMAT_A2B10G10R10_SINT_PACK32,     { 4, 4}},
    {VK_FORMAT_R16_UNORM,                   { 2, 1}},
    {VK_FORMAT_R16_SNORM,                   { 2, 1}},
    {VK_FORMAT_R16_USCALED,                 { 2, 1}},
    {VK_FORMAT_R16_SSCALED,                 { 2, 1}},
    {VK_FORMAT_R16_UINT,                    { 2, 1}},
    {VK_FORMAT_R16_SINT,                    { 2, 1}},
    {VK_FORMAT_R16_SFLOAT,                  { 2, 1}},
    {VK_FORMAT_R16G16_UNORM,                { 4, 2}},
    {VK_FORMAT_R16G16_SNORM,                { 4, 2}},
    {VK_FORMAT_R16G16_USCALED,              { 4, 2}},
    {VK_FORMAT_R16G16_SSCALED,              { 4, 2}},
    {VK_FORMAT_R16G16_UINT,                 { 4, 2}},
    {VK_FORMAT_R16G16_SINT,                 { 4, 2}},
    {VK_FORMAT_R16G16_SFLOAT,               { 4, 2}},
    {VK_FORMAT_R16G16B16_UNORM,             { 6, 3}},
    {VK_FORMAT_R16G16B16_SNORM,             { 6, 3}},
    {VK_FORMAT_R16G16B16_USCALED,           { 6, 3}},
    {VK_FORMAT_R16G16B16_SSCALED,           { 6, 3}},
    {VK_FORMAT_R16G16B16_UINT,              { 6, 3}},
    {VK_FORMAT_R16G16B16_SINT,              { 6, 3}},
    {VK_FORMAT_R16G16B16_SFLOAT,            { 6, 3}},
    {VK_FORMAT_R16G16B16A16_UNORM,          { 8, 4}},
    {VK_FORMAT_R16G16B16A16_SNORM,          { 8, 4}},
    {VK_FORMAT_R16G16B16A16_USCALED,        { 8, 4}},
    {VK_FORMAT_R16G16B16A16_SSCALED,        { 8, 4}},
    {VK_FORMAT_R16G16B16A16_UINT,           { 8, 4}},
    {VK_FORMAT_R16G16B16A16_SINT,           { 8, 4}},
    {VK_FORMAT_R16G16B16A16_SFLOAT,         { 8, 4}},
    {VK_FORMAT_R32_UINT,                    { 4, 1}},
    {VK_FORMAT_R32_SINT,                    { 4, 1}},
    {VK_FORMAT_R32_SFLOAT,                  { 4, 1}},
    {VK_FORMAT_R32G32_UINT,                 { 8, 2}},
    {VK_FORMAT_R32G32_SINT,                 { 8, 2}},
    {VK_FORMAT_R32G32_SFLOAT,               { 8, 2}},
    {VK_FORMAT_R32G32B32_UINT,              {12, 3}},
    {VK_FORMAT_R32G32B32_SINT,              {12, 3}},
    {VK_FORMAT_R32G32B32_SFLOAT,            {12, 3}},
    {VK_FORMAT_R32G32B32A32_UINT,           {16, 4}},
    {VK_FORMAT_R32G32B32A32_SINT,           {16, 4}},
    {VK_FORMAT_R32G32B32A32_SFLOAT,         {16, 4}},
    {VK_FORMAT_R64_UINT,                    { 8, 1}},
    {VK_FORMAT_R64_SINT,                    { 8, 1}},
    {VK_FORMAT_R64_SFLOAT,                  { 8, 1}},
    {VK_FORMAT_R64G64_UINT,                 {16, 2}},
    {VK_FORMAT_R64G64_SINT,                 {16, 2}},
    {VK_FORMAT_R64G64_SFLOAT,               {16, 2}},
    {VK_FORMAT_R64G64B64_UINT,              {24, 3}},
    {VK_FORMAT_R64G64B64_SINT,              {24, 3}},
    {VK_FORMAT_R64G64B64_SFLOAT,            {24, 3}},
    {VK_FORMAT_R64G64B64A64_UINT,           {32, 4}},
    {VK_FORMAT_R64G64B64A64_SINT,           {32, 4}},
    {VK_FORMAT_R64G64B64A64_SFLOAT,         {32, 4}},
    {VK_FORMAT_B10G11R11_UFLOAT_PACK32,     { 4, 3}},
    {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,      { 4, 3}},
    {VK_FORMAT_D16_UNORM,                   { 2, 1}},
    {VK_FORMAT_X8_D24_UNORM_PACK32,         { 4, 1}},
    {VK_FORMAT_D32_SFLOAT,                  { 4, 1}},
    {VK_FORMAT_S8_UINT,                     { 1, 1}},
    {VK_FORMAT_D16_UNORM_S8_UINT,           { 3, 2}},
    {VK_FORMAT_D24_UNORM_S8_UINT,           { 4, 2}},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,          { 8, 2}},
    {VK_FORMAT_BC1_RGB_UNORM_BLOCK,         { 8, 4}},
    {VK_FORMAT_BC1_RGB_SRGB_BLOCK,          { 8, 4}},
    {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        { 8, 4}},
    {VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         { 8, 4}},
    {VK_FORMAT_BC2_UNORM_BLOCK,             {16, 4}},
    {VK_FORMAT_BC2_SRGB_BLOCK,              {16, 4}},
    {VK_FORMAT_BC3_UNORM_BLOCK,             {16, 4}},
    {VK_FORMAT_BC3_SRGB_BLOCK,              {16, 4}},
    {VK_FORMAT_BC4_UNORM_BLOCK,             { 8, 4}},
    {VK_FORMAT_BC4_SNORM_BLOCK,             { 8, 4}},
    {VK_FORMAT_BC5_UNORM_BLOCK,             {16, 4}},
    {VK_FORMAT_BC5_SNORM_BLOCK,             {16, 4}},
    {VK_FORMAT_BC6H_UFLOAT_BLOCK,           {16, 4}},
    {VK_FORMAT_BC6H_SFLOAT_BLOCK,           {16, 4}},
    {VK_FORMAT_BC7_UNORM_BLOCK,             {16, 4}},
    {VK_FORMAT_BC7_SRGB_BLOCK,              {16, 4}},
    {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     { 8, 3}},
    {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      { 8, 3}},
    {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   { 8, 4}},
    {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    { 8, 4}},
    {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   {16, 4}},
    {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    {16, 4}},
    {VK_FORMAT_EAC_R11_UNORM_BLOCK,         { 8, 1}},
    {VK_FORMAT_EAC_R11_SNORM_BLOCK,         { 8, 1}},
    {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      {16, 2}},
    {VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      {16, 2}},
    {VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         {16, 4}},
    {VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT,   {16, 4}},
    {VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT,  {16, 4}},
    {VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT,  {16, 4}},
    {VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        {16, 4}},
    {VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT,  {16, 4}},
    {VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      {16, 4}},
    {VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT, {16, 4}},
    {VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      {16, 4}},
    {VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT, {16, 4}},
    {VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      {16, 4}},
    {VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       {16, 4}},
    {VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT, {16, 4}},
    {VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, { 8, 4}},
    {VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, { 8, 4}},
    {VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, { 8, 4}},
    {VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, { 8, 4}},
    {VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,  { 8, 4}},
    {VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,  { 8, 4}},
    {VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,  { 8, 4}},
    {VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,  { 8, 4}},
    // KHR_sampler_YCbCr_conversion extension - single-plane variants
    // 'PACK' formats are normal, uncompressed
    {VK_FORMAT_R10X6_UNORM_PACK16,                          {2, 1}},
    {VK_FORMAT_R10X6G10X6_UNORM_2PACK16,                    {4, 2}},
    {VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,          {8, 4}},
    {VK_FORMAT_R12X4_UNORM_PACK16,                          {2, 1}},
    {VK_FORMAT_R12X4G12X4_UNORM_2PACK16,                    {4, 2}},
    {VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,          {8, 4}},
    // _422 formats encode 2 texels per entry with B, R components shared - treated as compressed w/ 2x1 block size
    {VK_FORMAT_G8B8G8R8_422_UNORM,                          {4, 4}},
    {VK_FORMAT_B8G8R8G8_422_UNORM,                          {4, 4}},
    {VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,      {8, 4}},
    {VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,      {8, 4}},
    {VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,      {8, 4}},
    {VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,      {8, 4}},
    {VK_FORMAT_G16B16G16R16_422_UNORM,                      {8, 4}},
    {VK_FORMAT_B16G16R16G16_422_UNORM,                      {8, 4}},
    // KHR_sampler_YCbCr_conversion extension - multi-plane variants
    // Formats that 'share' components among texels (_420 and _422), size represents total bytes for the smallest possible texel block
    // _420 share B, R components within a 2x2 texel block
    {VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,                   { 6, 3}},
    {VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,                    { 6, 3}},
    {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,  {12, 3}},
    {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,   {12, 3}},
    {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,  {12, 3}},
    {VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,   {12, 3}},
    {VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,                {12, 3}},
    {VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,                 {12, 3}},
    // _422 share B, R components within a 2x1 texel block
    {VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,                   {4, 3}},
    {VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,                    {4, 3}},
    {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,  {8, 3}},
    {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,   {8, 3}},
    {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,  {8, 3}},
    {VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,   {8, 3}},
    {VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,                {8, 3}},
    {VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,                 {8, 3}},
    // _444 do not share
    {VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,                     {3, 3}},
    {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,    {6, 3}},
    {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,    {6, 3}},
    {VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,                  {6, 3}},
    {VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT,                  {3, 3}},
    {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT, {6, 3}},
    {VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT, {6, 3}},
    {VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT,               {6, 3}},
};
// clang-format on

VmaMemoryUsage toVmaMemoryUsage( MemoryUsage value )
{
	// clang-format off
    switch (value) {
        default: break;
	    case MemoryUsage::CPU_ONLY   : return VMA_MEMORY_USAGE_CPU_ONLY; break;
	    case MemoryUsage::CPU_TO_GPU : return VMA_MEMORY_USAGE_CPU_TO_GPU; break;
	    case MemoryUsage::GPU_ONLY   : return VMA_MEMORY_USAGE_GPU_ONLY; break;
	    case MemoryUsage::GPU_TO_CPU : return VMA_MEMORY_USAGE_GPU_TO_CPU; break;
    };
	// clang-format on
	return VMA_MEMORY_USAGE_UNKNOWN;
}

VkSampleCountFlagBits toVkSampleCount( uint32_t samples )
{
	samples							  = ( samples == 0 ) ? 1 : samples;
	samples							  = ( samples > 2 ) ? grfx::roundUpPow2( samples ) : samples;
	VkSampleCountFlagBits sampleCount = static_cast<VkSampleCountFlagBits>( samples );
    return sampleCount;
}

uint32_t formatSize( VkFormat format )
{
	auto it = kFormatInfo.find( format );
	return ( it != kFormatInfo.end() ) ? it->second.size : 0;
}

VkImageAspectFlags determineAspectMask( VkFormat format )
{
	// clang-format off
    switch (format) {
        // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT: {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        } break;

        // Stencil
        case VK_FORMAT_S8_UINT: {
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        } break;

        // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT: {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        } break;
        
            // Assume everything else is color
        default: break;
    }
	// clang-format on
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkPipelineStageFlags guessPipelineStageFromImageLayout( vk::DeviceRef device, VkImageLayout layout, bool isSource )
{
	VkPipelineStageFlags pipelineStageFlags = 0;
	switch ( layout ) {
		default: break;

		case VK_IMAGE_LAYOUT_GENERAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		} break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		} break;

		// Depth   WRITE
		// Stencil WRITE
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   READ
		// Stencil READ
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
			pipelineStageFlags =
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			// Only add geometry and tessellation bits if geometryShader feature is
			// present - otherwise validation layers will complain.
			if ( device->getDeviceFeatures().geometryShader ) {
				pipelineStageFlags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			}
			if ( device->getDeviceFeatures().tessellationShader ) {
				pipelineStageFlags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			}
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} break;

		// Depth   READ
		// Stencil WRITE
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   WRITE
		// Stencil READ
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   WRITE
		// Stencil n/a
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   READ
		// Stencil n/a
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   n/a
		// Stencil WRITE
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		// Depth   n/a
		// Stencil READ
		case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL: {
			pipelineStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		} break;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
			pipelineStageFlags = isSource ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		} break;
	}
	return pipelineStageFlags;
}

VkResult cmdTransitionImageLayout(
	PFN_vkCmdPipelineBarrier fnCmdPipelineBarrier,
	VkCommandBuffer			 commandBuffer,
	VkImage					 image,
	VkImageAspectFlags		 aspectMask,
	uint32_t				 baseMipLevel,
	uint32_t				 levelCount,
	uint32_t				 baseArrayLayer,
	uint32_t				 layerCount,
	VkImageLayout			 oldLayout,
	VkImageLayout			 newLayout,
	VkPipelineStageFlags	 newPipelineStageFlags )
{
	VkPipelineStageFlags srcStageMask	 = 0;
	VkPipelineStageFlags dstStageMask	 = 0;
	VkAccessFlags		 srcAccessMask	 = 0;
	VkAccessFlags		 dstAccessMask	 = 0;
	VkDependencyFlags	 dependencyFlags = 0;

	switch ( oldLayout ) {
		default: {
			throw VulkanExc( "invalid value for oldLayout" );
			return VK_ERROR_INITIALIZATION_FAILED;
		} break;

		case VK_IMAGE_LAYOUT_UNDEFINED: {
			srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_GENERAL: {
			// @TODO: This may need tweaking.
			srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
			srcStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED: {
			srcStageMask  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
			// @TODO: This may need tweaking.
			srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
			// @TODO: This may need tweaking.
			srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
			srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		} break;
	}

	switch ( newLayout ) {
		default: {
			// Note: VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED cannot be a destination layout.
			throw VulkanExc( "invalid value for newLayout" );
			return VK_ERROR_INITIALIZATION_FAILED;
		} break;

		case VK_IMAGE_LAYOUT_GENERAL: {
			dstStageMask  = newPipelineStageFlags;
			dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
			dstStageMask  = newPipelineStageFlags;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			newLayout	  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		} break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
			dstStageMask  = newPipelineStageFlags;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		} break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
			dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		} break;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
			dstStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstAccessMask = 0;
		} break;
	}

	VkImageMemoryBarrier barrier			= { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask					= srcAccessMask;
	barrier.dstAccessMask					= dstAccessMask;
	barrier.oldLayout						= oldLayout;
	barrier.newLayout						= newLayout;
	barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.image							= image;
	barrier.subresourceRange.aspectMask		= aspectMask;
	barrier.subresourceRange.baseMipLevel	= baseMipLevel;
	barrier.subresourceRange.levelCount		= levelCount,
	barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount		= layerCount;

	fnCmdPipelineBarrier(
		commandBuffer,	 // commandBuffer
		srcStageMask,	 // srcStageMask
		dstStageMask,	 // dstStageMask
		dependencyFlags, // dependencyFlags
		0,				 // memoryBarrierCount
		nullptr,		 // pMemoryBarriers
		0,				 // bufferMemoryBarrierCount
		nullptr,		 // pBufferMemoryBarriers
		1,				 // imageMemoryBarrierCount
		&barrier );		 // pImageMemoryBarriers);

	return VK_SUCCESS;
}

} // namespace cinder::vk
