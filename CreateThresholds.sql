USE [UGMon]
GO

/****** Object:  Table [dbo].[FSThresholds]    Script Date: 02/11/2011 10:32:26 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[FSThresholds](
	[ID] [int] IDENTITY(1,1) NOT NULL,
	[EvalOrder] [int] NOT NULL,
	[HostExpression] [nvarchar](max) NOT NULL,
	[VolumeExpression] [nvarchar](max) NOT NULL,
	[TreshFreeMBWarn] [int] NOT NULL,
	[ThreshFreeMBHigh] [int] NOT NULL,
	[ThreshUtilWarn] [float] NOT NULL,
	[ThreshUtilHigh] [float] NOT NULL,
	[WarnSev] [nvarchar](20) NOT NULL,
	[highSev] [nvarchar](20) NOT NULL,
 CONSTRAINT [PK_FSThresholds] PRIMARY KEY CLUSTERED 
(
	[ID] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

ALTER TABLE [dbo].[FSThresholds] ADD  CONSTRAINT [DF_FSThresholds_HostExpression]  DEFAULT ('<*>') FOR [HostExpression]
GO

ALTER TABLE [dbo].[FSThresholds] ADD  CONSTRAINT [DF_FSThresholds_VolumeExpression]  DEFAULT ('<*>') FOR [VolumeExpression]
GO

ALTER TABLE [dbo].[FSThresholds] ADD  CONSTRAINT [DF_Thresholds_WarnSev]  DEFAULT (N'Minor') FOR [WarnSev]
GO

ALTER TABLE [dbo].[FSThresholds] ADD  CONSTRAINT [DF_Thresholds_highSev]  DEFAULT (N'Major') FOR [highSev]
GO

